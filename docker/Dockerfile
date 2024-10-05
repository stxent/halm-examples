FROM opensuse/tumbleweed

RUN zypper ref
RUN zypper in -y git cmake gcc libuv-devel check-devel lcov python311-pip && zypper cc -a
RUN zypper in -y wget xz && wget https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-eabi.tar.xz -O /tmp/gcc-arm-none-eabi.tar.xz && mkdir -p /build/gcc-arm-none-eabi && tar -xvf /tmp/gcc-arm-none-eabi.tar.xz -C /build/gcc-arm-none-eabi --strip 1 && rm /tmp/gcc-arm-none-eabi.tar.xz && zypper rm -y wget xz && zypper cc -a
RUN mkdir -p /build/pyenv && python3 -m venv /build/pyenv && /build/pyenv/bin/pip3 install jinja2 kconfiglib
ENV PATH="$PATH:/build/gcc-arm-none-eabi/bin:/build/pyenv/bin"

RUN mkdir -p /build && cd /build && git clone --depth=1 https://github.com/stxent/halm-examples.git && cd halm-examples && git submodule update --init --recursive
COPY run.sh /build/run.sh

ENTRYPOINT ["/build/run.sh"]
