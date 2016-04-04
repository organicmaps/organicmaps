FROM debian:jessie
MAINTAINER Andrey Ivanov stayhardordie@gmail.com

ENV REPOSITORY=https://github.com/mapsme/omim.git \
    DIR=/srv

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    wget \
    git \
    libssl-dev \
    clang \
    libc++-dev \
    libglu1-mesa-dev \
    libstdc++-4.8-dev \
    qt5-default \
    cmake \
    libboost-all-dev \
    mesa-utils \
    libtbb2 \
    libtbb-dev \
    libluabind-dev \
    libluabind0.9.1 \
    lua5.1 \
    osmpbf-bin \
    libprotobuf-dev \
    libstxxl-dev \
    libxml2-dev \
    libsparsehash-dev \
    libbz2-dev \
    zlib1g-dev \
    libzip-dev \    
    liblua5.1-0-dev \
    pkg-config \
    libgdal-dev \
    libexpat1-dev \
    libosmpbf-dev
WORKDIR $DIR
RUN git clone --depth=1 --recursive $REPOSITORY
RUN cd omim && \
    echo | ./configure.sh && \
    cd ../
RUN CONFIG=gtool omim/tools/unix/build_omim.sh -cro
CMD ["/bin/bash"]
