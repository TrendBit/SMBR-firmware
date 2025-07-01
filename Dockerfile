FROM alpine:3.18

# Install toolchain


RUN apk update && \
    apk upgrade && \
    apk add \
            autoconf \
            automake \
            bash \
            bsd-compat-headers \
            build-base \
            cmake \
            g++-arm-none-eabi \
            g++\
            gcc-arm-none-eabi \
            gcc\
            gdb-multiarch \
            git \
            libtool \
            libusb-dev \
            linux-headers \
            newlib-arm-none-eabi \
            ninja \
            pkgconf \
            py3-pip \
            python3 \
            texinfo \
            zip

RUN pip install --break-system-packages kconfiglib

RUN cd /usr/share/ &&\
    git clone https://github.com/linux-test-project/lcov.git --branch v1.16 lcov/ && \
    cd lcov && \
    make install

# openocd
RUN cd /usr/share/ &&\
    git clone https://github.com/raspberrypi/openocd.git --branch rp2040-v0.12.0 --depth=1 && \
    cd openocd &&\
    ./bootstrap &&\
    ./configure &&\
    make -j"$(nproc)" &&\
    make install

RUN cd ..
RUN rm -rf /usr/share/openocd

EXPOSE 3333

# Raspberry Pi Pico SDK
ARG SDK_PATH=/usr/share/pico_sdk
ENV PICO_SDK_PATH=$SDK_PATH

RUN git clone --recursive --branch 2.1.1 https://github.com/raspberrypi/pico-sdk $SDK_PATH && \
    cd $SDK_PATH && \
    git submodule update --init

ENV UDEV=1

# Picotool installation
RUN git clone --depth 1 --branch 2.1.0 https://github.com/raspberrypi/picotool.git /home/user && \
    cd /home/user && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make && \
    make install

ARG USER_ID
ARG GROUP_ID

RUN addgroup --gid $GROUP_ID user
RUN adduser --disabled-password --gecos '' -S user --uid $USER_ID -G user

WORKDIR /project
