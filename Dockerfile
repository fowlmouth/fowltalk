FROM ubuntu:20.04

RUN apt-get update && apt-get install -y build-essential automake git

WORKDIR /src

ADD ./src ./src
ADD ./include ./include
ADD ./tests ./tests
ADD Makefile run-tests.sh ./

RUN make -j"$(nproc)" && make cleanobjs
