FROM ubuntu
MAINTAINER Wieger Wesselink <j.w.wesselink@tue.nl>

RUN apt-get update && apt-get install -y \
 build-essential \
 cmake \
 libboost-dev \
 git

RUN git clone https://github.com/wiegerw/mackerel.git
RUN cd mackerel \
    && cmake . -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=`pwd`/install \
    && make -j4 \
    && make install
