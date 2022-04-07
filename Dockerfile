# Copyright (C) 2022  Christian Berger
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Part to build opendlv-video-h264-decoder.
FROM ubuntu:20.04 as builder
MAINTAINER Christian Berger "christian.berger@gu.se"
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get dist-upgrade -y && \
    apt-get install -y --no-install-recommends \
        ca-certificates \
        cmake \
        make \
        linux-headers-generic \
        git \
        libx11-dev \
        nasm \
        clang \
        wget
RUN cd tmp && \
    git clone https://chromium.googlesource.com/libyuv/libyuv && \
    cd libyuv &&\
    git checkout -b working eb6e7bb63738e29efd82ea3cf2a115238a89fa51 && \
    make -f linux.mk CXX=clang++ libyuv.a && if [ `uname -m` = aarch64 ] ; then cp libyuv.a /usr/lib/aarch64-linux-gnu ; else cp libyuv.a /usr/lib/x86_64-linux-gnu ; fi && cd include && cp -r * /usr/include
RUN cd tmp && \
    git clone --depth 1 --branch v2.2.0 https://github.com/cisco/openh264.git && \
    cd openh264 && mkdir b && cd b && \
    ln -sf /usr/bin/clang++-10 /usr/bin/g++ && \
    ln -sf /usr/bin/clang-10 /usr/bin/cc && \
    if [ `uname -m` = aarch64 ] ; then make ARCH=arm64 -j2 -f ../Makefile libraries ; else make -j2 -f ../Makefile libraries ; fi && make -f ../Makefile install
ADD . /opt/sources
WORKDIR /opt/sources
RUN mkdir build && \
    cd build && \
    cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=/tmp .. && \
    make && make install
RUN cd /tmp && if [ `uname -m` = aarch64 ] ; then wget http://ciscobinary.openh264.org/libopenh264-2.2.0-linux-arm64.6.so.bz2 && mv libopenh264-2.2.0-linux-arm64.6.so.bz2 libopenh264.so.6.bz2 ; else wget http://ciscobinary.openh264.org/libopenh264-2.2.0-linux64.6.so.bz2 && mv libopenh264-2.2.0-linux64.6.so.bz2 libopenh264.so.6.bz2 ; fi

# Part to deploy opendlv-video-h264-decoder.
FROM ubuntu:20.04
MAINTAINER Christian Berger "christian.berger@gu.se"

RUN apt-get update -y && \
    apt-get upgrade -y && \
    apt-get dist-upgrade -y && \
    apt-get install -y --no-install-recommends libx11-6

WORKDIR /tmp
COPY --from=builder /tmp/libopenh264.so.6.bz2 .
RUN bunzip2 libopenh264.so.6.bz2 && \
    if [ `uname -m` = aarch64 ] ; then mv /tmp/libopenh264.so.6 /usr/lib/aarch64-linux-gnu/libopenh264.so.6 ; else mv /tmp/libopenh264.so.6 /usr/lib/x86_64-linux-gnu/libopenh264.so.6 ; fi

WORKDIR /usr/bin
COPY --from=builder /tmp/bin/opendlv-video-h264-decoder .
ENTRYPOINT ["/usr/bin/opendlv-video-h264-decoder"]
