FROM ubuntu:18.04

RUN apt-get update \
    && apt-get install -y --no-install-recommends git wget make g++ libssl1.0-dev ca-certificates \
    && rm -rf /var/lib/apt/lists/*

ENV version 3.17
ENV build 0
RUN mkdir ~/temp \
    && cd ~/temp \
    && wget --no-check-certificate https://cmake.org/files/v$version/cmake-$version.$build.tar.gz \
    && tar -xzvf cmake-$version.$build.tar.gz \
    && cd cmake-$version.$build \
    && ./bootstrap \
    && make -j4 \
    && make install

RUN apt-get update \
    && apt install -y ca-certificates \
    && rm -rf /var/lib/apt/lists/*

ARG CACHE_DATE=not_a_date

RUN git clone --depth=1 -b experimental https://github.com/skypjack/uvw.git \
    && cd uvw \
    && mkdir -p uvw-build && cd uvw-build \
    && cmake .. -D BUILD_TESTING=ON \
    && make -j4 \
    && CTEST_OUTPUT_ON_FAILURE=TRUE ctest --timeout 5 -C Debug -j4
