/*
 * Copyright (C) 2018  Christian Berger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cluon-complete.hpp"
#include "opendlv-standard-message-set.hpp"

#include <wels/codec_api.h>
#include <libyuv.h>
#include <X11/Xlib.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>

int32_t main(int32_t argc, char **argv) {
    int32_t retCode{1};
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ( (0 == commandlineArguments.count("cid")) ||
         (0 == commandlineArguments.count("name")) ) {
        std::cerr << argv[0] << " listens for H264 frames in an OD4Session to decode as ARGB image data into a shared memory area." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --cid=<OpenDaVINCI session> --name=<name of shared memory area> [--verbose]" << std::endl;
        std::cerr << "         --cid:     CID of the OD4Session to listen for h264 frames" << std::endl;
        std::cerr << "         --id:      when using several instances, only decode h264 with this senderStamp" << std::endl;
        std::cerr << "         --name:    name of the shared memory area to create" << std::endl;
        std::cerr << "         --verbose: print decoding information and display image" << std::endl;
        std::cerr << "Example: " << argv[0] << " --cid=111 --name=data --verbose" << std::endl;
    }
    else {
        const std::string NAME{commandlineArguments["name"]};
        const bool VERBOSE{commandlineArguments.count("verbose") != 0};
        const uint32_t ID{(commandlineArguments["id"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["id"])) : 0};

        ISVCDecoder *decoder{nullptr};
        WelsCreateDecoder(&decoder);
        if (0 != WelsCreateDecoder(&decoder) && (nullptr != decoder)) {
            std::cerr << argv[0] << ": Failed to create openh264 decoder." << std::endl;
            return retCode;
        }

        int logLevel{VERBOSE ? WELS_LOG_INFO : WELS_LOG_QUIET};
        decoder->SetOption(DECODER_OPTION_TRACE_LEVEL, &logLevel);

        SDecodingParam decodingParam;
        {
            memset(&decodingParam, 0, sizeof(SDecodingParam));
            decodingParam.eEcActiveIdc = ERROR_CON_DISABLE;
            decodingParam.bParseOnly = false;
            decodingParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;
        }
        if (cmResultSuccess != decoder->Initialize(&decodingParam)) {
            std::cerr << argv[0] << ": Failed to initialize openh264 decoder." << std::endl;
            return retCode;
        }

        // Interface to a running OpenDaVINCI session (ignoring any incoming Envelopes).
        cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"]))};

        std::unique_ptr<cluon::SharedMemory> sharedMemory(nullptr);

        Display *display{nullptr};
        Visual *visual{nullptr};
        Window window{0};
        XImage *ximage{nullptr};

        auto onNewImage = [&decoder, &sharedMemory, &display, &visual, &window, &ximage, &NAME, &VERBOSE, &ID](cluon::data::Envelope &&env){
            if (ID == env.senderStamp()) {
                opendlv::proxy::ImageReading img = cluon::extractMessage<opendlv::proxy::ImageReading>(std::move(env));
                if ("h264" == img.format()) {
                    const uint32_t WIDTH = img.width();
                    const uint32_t HEIGHT = img.height();

                    if (!sharedMemory) {
                        std::clog << "[opendlv-video-h264-decoder]: Created shared memory " << NAME << " (" << (WIDTH * HEIGHT * 4) << " bytes) for an ARGB image (width = " << WIDTH << ", height = " << HEIGHT << ")." << std::endl;
                        sharedMemory.reset(new cluon::SharedMemory{NAME, WIDTH * HEIGHT * 4});
                        if (VERBOSE) {
                            display = XOpenDisplay(NULL);
                            visual = DefaultVisual(display, 0);
                            window = XCreateSimpleWindow(display, RootWindow(display, 0), 0, 0, WIDTH, HEIGHT, 1, 0, 0);
                            ximage = XCreateImage(display, visual, 24, ZPixmap, 0, reinterpret_cast<char*>(sharedMemory->data()), WIDTH, HEIGHT, 32, 0);
                            XMapWindow(display, window);
                        }
                    }
                    if (sharedMemory) {
                        uint8_t* yuvData[3];

                        SBufferInfo bufferInfo;
                        memset(&bufferInfo, 0, sizeof (SBufferInfo));

                        std::string data{img.data()};
                        const uint32_t LEN{static_cast<uint32_t>(data.size())};

                        if (0 != decoder->DecodeFrame2(reinterpret_cast<const unsigned char*>(data.c_str()), LEN, yuvData, &bufferInfo)) {
                            std::cerr << "H264 decoding for current frame failed." << std::endl;
                        }
                        else {
                            if (1 == bufferInfo.iBufferStatus) {
                                sharedMemory->lock();
                                {
                                    libyuv::I420ToARGB(yuvData[0], bufferInfo.UsrData.sSystemBuffer.iStride[0], yuvData[1], bufferInfo.UsrData.sSystemBuffer.iStride[1], yuvData[2], bufferInfo.UsrData.sSystemBuffer.iStride[1], reinterpret_cast<uint8_t*>(sharedMemory->data()), WIDTH * 4, WIDTH, HEIGHT);
                                    if (VERBOSE) {
                                        XPutImage(display, window, DefaultGC(display, 0), ximage, 0, 0, 0, 0, WIDTH, HEIGHT);
                                    }
                                }
                                sharedMemory->unlock();
                                sharedMemory->notifyAll();
                            }
                        }
                    }
                }
            }
        };

        // Register lambda to handle incoming frames.
        od4.dataTrigger(opendlv::proxy::ImageReading::ID(), onNewImage);

        while (od4.isRunning()) {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
        }

        if (decoder) {
            decoder->Uninitialize();
            WelsDestroyDecoder(decoder);
        }

        if (VERBOSE) {
            XCloseDisplay(display);
        }
        retCode = 0;
    }
    return retCode;
}

