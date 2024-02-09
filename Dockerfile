FROM ubuntu:20.04

ENV DEBIAN_FRONTEND noninteractive
ENV PATH="${PATH}:/opt/KasperskyOS-Community-Edition-1.1.1.40/toolchain/bin"

COPY KasperskyOS-Community-Edition_1.1.1.40_en.deb /kos/

RUN apt-get update \
    && apt install -y --fix-broken /kos/KasperskyOS-Community-Edition_1.1.1.40_en.deb iproute2 iputils-ping curl mc \
    && adduser --disabled-password --gecos "" user \
    && echo 'user ALL=(ALL) NOPASSWD: ALL' >> /etc/sudoers

#COPY run_script.sh /home/user/
#RUN chmod +x /home/user/run_script.sh

#CMD ["bash"]
