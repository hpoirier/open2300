FROM debian:bullseye-slim
LABEL maintainer="Herve Poirier <herve.poirier@gmail.com>"
# Build with
# docker build -t hpoirier/open2300:2022-05-26 -f Dockerfile .

RUN DEBIAN_FRONTEND="noninteractive" apt-get update && apt-get install -y \
    git \
    build-essential \
    libmosquitto-dev

RUN git clone -b master https://github.com/hpoirier/open2300
WORKDIR open2300
RUN make

RUN    DEBIAN_FRONTEND="noninteractive" apt-get -qq autopurge $PACKAGES && \
    apt-get clean && \
    rm -r /var/lib/apt/lists /var/cache/apt

VOLUME /etc/open2300

COPY open2300.conf open2300.conf.orig
COPY entrypoint.sh /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
