# Create a Docker image with every tools required to build and deploy infrabase
#
# Copyright (c) 2025 REDS Institute, HEIG-VD
#
# Image build:
# $ docker build -t micofe-build .
#
# Running the image in interative mode (allowing to run infrabase commands directly in it)
# $ docker run --privileged -v /dev:/dev -v $(pwd):$(pwd) -w $(pwd) --rm -it micofe-build:latest

FROM ubuntu:24.04 AS toolchains

# Download and extract AARCH64 toolchain
RUN apt-get update && apt-get install -y xz-utils wget

RUN wget -O /tmp/aarch64-none-linux-gnu.tar.xz "https://developer.arm.com/-/media/Files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-$(uname -m)-aarch64-none-linux-gnu.tar.xz"
RUN wget -O /tmp/aarch64-none-elf.tar.xz "https://developer.arm.com/-/media/Files/downloads/gnu/12.2.rel1/binrel/arm-gnu-toolchain-12.2.rel1-$(uname -m)-aarch64-none-elf.tar.xz"

RUN mkdir /tmp/aarch64-none-linux-gnu
RUN tar xf /tmp/aarch64-none-linux-gnu.tar.xz -C /tmp/aarch64-none-linux-gnu

RUN mkdir /tmp/aarch64-none-elf
RUN tar xf /tmp/aarch64-none-elf.tar.xz -C /tmp/aarch64-none-elf

FROM ubuntu:24.04

# Change used locale for BitBake
RUN apt-get update && apt-get install -y locales && rm -rf /var/lib/apt/lists/* \
	&& localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8
ENV LANG=en_US.utf8

# Install require dependencies and tools
RUN apt-get update && apt-get install -y sudo python3 git file wget xz-utils python3-pip \
	build-essential ninja-build pkg-config libglib2.0-dev libsdl2-dev fdisk \
	cpio unzip rsync bc bzip2 python3-venv \
	make cmake gcc-arm-none-eabi libc-dev \
	bison flex bash patch mount device-tree-compiler \
	dosfstools u-boot-tools net-tools \
	bridge-utils iptables dnsmasq libssl-dev \
	util-linux e2fsprogs vim nano libncurses-dev \
	&& rm -rf /var/lib/apt/lists/*

RUN pip install --break-system-packages sphinxcontrib-openapi sphinxcontrib-plantuml

# Make sudo usable without password
RUN usermod -aG sudo ubuntu
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# Copy extracted toolchains
RUN mkdir -p /opt/toolchains/
COPY --from=toolchains /tmp/aarch64-none-linux-gnu/* /opt/toolchains/aarch64-none-linux-gnu
COPY --from=toolchains /tmp/aarch64-none-elf/* /opt/toolchains/aarch64-none-elf
ENV PATH="$PATH:/opt/toolchains/aarch64-none-linux-gnu/bin/:/opt/toolchains/aarch64-none-elf/bin/"

USER ubuntu
WORKDIR /home/ubuntu/src
