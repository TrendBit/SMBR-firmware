FROM alpine:3.18

# Install toolchain


RUN apk update && \
    apk upgrade && \
    apk add git \
            python3 \
            py3-pip \
            cmake \
            build-base \
            libusb-dev \
            libtool \
            texinfo \
            automake\
            autoconf \
            bsd-compat-headers \
            gcc\
            g++\
            newlib-arm-none-eabi \
            g++-arm-none-eabi \
            gcc-arm-none-eabi \
            gdb-multiarch \
            bash \
            ninja

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
    #remove unneeded directories
RUN cd ..
RUN rm -rf /usr/share/openocd

EXPOSE 3333

# Raspberry Pi Pico SDK
ARG SDK_PATH=/usr/share/pico_sdk
RUN git clone --depth 1 --branch 1.5.0 https://github.com/raspberrypi/pico-sdk $SDK_PATH && \
    cd $SDK_PATH && \
    git submodule update --init

ENV UDEV=1

ENV PICO_SDK_PATH=$SDK_PATH

# Picotool installation
RUN git clone --depth 1 --branch 1.1.1 https://github.com/raspberrypi/picotool.git /home/picotool && \
    cd /home/picotool && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make && \
    cp /home/picotool/build/picotool /bin/picotool && \
    rm -rf /home/picotool

ARG USER_ID
ARG GROUP_ID

RUN addgroup --gid $GROUP_ID user
RUN adduser --disabled-password --gecos '' -S user --uid $USER_ID -G user

WORKDIR /project
