## OpenDLV Microservice to decode video frames from h264 into a shared memory

This repository provides source code to decode broadcasted video frames into a
shared memory area for the OpenDLV software ecosystem.

[![License: GPLv3](https://img.shields.io/badge/license-GPL--3-blue.svg
)](https://www.gnu.org/licenses/gpl-3.0.txt)

[OpenH264 Video Codec provided by Cisco Systems, Inc.](https://www.openh264.org/faq.html)

During the Docker-ized build process for this microservice, Cisco's binary
library is downloaded from Cisco's webserver and installed on the user's
computer due to legal implications arising from the patents around the [AVC/h264 format](http://www.mpegla.com/main/programs/avc/pages/intro.aspx).

End user's notice according to [AVC/H.264 Patent Portfolio License Conditions](https://www.openh264.org/BINARY_LICENSE.txt):
**When you are using this software and build scripts from this repository, you are agreeing to and obeying the terms under which Cisco is making the binary library available.**


## Table of Contents
* [Dependencies](#dependencies)
* [Building and Usage](#building-and-usage)
* [License](#license)


## Dependencies
You need a C++14-compliant compiler to compile this project.

The following dependency is part of the source distribution:
* [libcluon](https://github.com/chrberger/libcluon) - [![License: GPLv3](https://img.shields.io/badge/license-GPL--3-blue.svg
)](https://www.gnu.org/licenses/gpl-3.0.txt)

The following dependencies are will be downloaded and installed during the Docker-ized build:
* [openh264](https://www.openh264.org/index.html) - [![License: BSD 2-Clause](https://img.shields.io/badge/License-BSD%202--Clause-blue.svg)](https://opensource.org/licenses/BSD-2-Clause) - [AVC/H.264 Patent Portfolio License Conditions](https://www.openh264.org/BINARY_LICENSE.txt)
* [libyuv](https://chromium.googlesource.com/libyuv/libyuv/+/master) - [![License: BSD 3-Clause](https://img.shields.io/badge/License-BSD%203--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause) - [Google Patent License Conditions](https://chromium.googlesource.com/libyuv/libyuv/+/master/PATENTS)

## Building and Usage
Due to legal implications arising from the patents around the [AVC/h264 format](http://www.mpegla.com/main/programs/avc/pages/intro.aspx),
we cannot provide and distribute pre-built Docker images. Therefore, we provide
the build instructions in a `Dockerfile` that can be easily integrated in a
`docker-compose.yml` file.

To run this microservice using `docker-compose`, you can simply add the following
section to your `docker-compose.yml` file to let Docker build this software for you:

```yml
    video-h264-decoder-amd64:
        build:
            context: https://github.com/chalmers-revere/opendlv-video-h264-decoder.git
            dockerfile: Dockerfile.amd64
        restart: on-failure
        network_mode: "host"
        ipc: "host"
        volumes:
        - /tmp:/tmp
        environment:
        - DISPLAY=${DISPLAY}
        command: "--cid=111 --name=imageData"
```

As this microservice is connecting to an OD4Session to receive h264 frames to
decode them into a shared memory area using SysV IPC, the `docker-compose.yml`
file specifies the use of `ipc:host`. The parameter `network_mode: "host"` is
necessary to receive h264 frames broadcast from other microservices running
in an `OD4Session` from OpenDLV. The folder `/tmp` is shared into the Docker
container to provide tokens describing the shared memory area.
The parameters to the application are:

* `--cid=111`: Identifier of the OD4Session to listen for h264 frames
* `--id=2`: Optional identifier to listen only for those h264 frames with the matching senderStamp of the OD4Session
* `--name=XYZ`: Name of the shared memory area to create for storing the ARGB image data
* `--verbose`: Display decoding information and render the image to screen (requires X11; run `xhost +` to allow access to you X11 server)


## License

* This project is released under the terms of the GNU GPLv3 License

