FROM mcr.microsoft.com/devcontainers/base:debian-12
#
# Ref: https://docs.zephyrproject.org/3.5.0/develop/getting_started/index.html
#

#
# Install dependencies
#
RUN apt-get update && \
	apt-get install -y --no-install-recommends \
	git cmake ninja-build gperf ccache dfu-util device-tree-compiler wget     \
	python3-dev python3-pip python3-setuptools python3-tk python3-wheel       \
	xz-utils file make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1 && \
	apt-get clean

#
# Get Zephyr and install Python dependencies
#

# TODO: Integrate line with before
RUN apt-get install -y python3-venv && apt-get clean

USER vscode
WORKDIR /home/vscode/

RUN python3 -m venv ~/zephyrproject/.venv
ENV PATH /home/vscode/zephyrproject/.venv/bin:/usr/local/bin:/usr/bin:/bin
RUN pip install west
RUN west init --mr v3.5.0 ~/zephyrproject

# Temporary zephyr base to the project root
ENV ZEPHYR_BASE=/home/vscode/zephyrproject/
RUN west update
RUN west zephyr-export
RUN pip install -r ~/zephyrproject/zephyr/scripts/requirements.txt
# Real zephyr base
ENV ZEPHYR_BASE=/home/vscode/zephyrproject/zephyr/

#
# Install Zephyr SDK
#
RUN wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.3/zephyr-sdk-0.16.3_linux-x86_64.tar.xz && \
	tar xvf zephyr-sdk-0.16.3_linux-x86_64.tar.xz && \
	rm zephyr-sdk-0.16.3_linux-x86_64.tar.xz

RUN ./zephyr-sdk-0.16.3/setup.sh -h -c
