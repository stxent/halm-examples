FROM opensuse/tumbleweed

RUN zypper ref
RUN zypper in -y wget tar gzip xz git cmake gcc findutils lcov valgrind libuv-devel check-devel python311-pip
RUN wget https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz -O /tmp/gcc-arm-none-eabi.tar.xz && mkdir -p /build/gcc-arm-none-eabi && tar -xvf /tmp/gcc-arm-none-eabi.tar.xz -C /build/gcc-arm-none-eabi --strip 1
RUN mkdir -p /build/pyenv && python3 -m venv /build/pyenv && /build/pyenv/bin/pip3 install jinja2 kconfiglib
ENV PATH="/build/pyenv/bin:/build/gcc-arm-none-eabi/bin:$PATH"
RUN mkdir -p /build && cd /build && git clone https://github.com/stxent/halm-examples.git && cd halm-examples && git submodule update --init --recursive
COPY run.sh /build/run.sh

ENTRYPOINT /build/run.sh
