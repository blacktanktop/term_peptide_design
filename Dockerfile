FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get install -y build-essential cmake clang git libc++-dev libc++abi-dev libssl-dev vim
ADD https://api.github.com/repos/Grigoryanlab/Mosaist/git/refs/heads/master version.json
RUN git clone -b master https://github.com/Grigoryanlab/Mosaist.git $GIT_HOME/
