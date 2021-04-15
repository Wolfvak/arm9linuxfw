FROM debian:buster-slim                                                                          
RUN apt-get update && apt-get install -y gcc-arm-none-eabi binutils-arm-none-eabi build-essential
WORKDIR /arm9linuxfw
