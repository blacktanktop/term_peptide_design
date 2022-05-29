FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get install -y build-essential cmake clang git libc++-dev libc++abi-dev libssl-dev vim wget
ADD https://api.github.com/repos/Grigoryanlab/Mosaist/git/refs/heads/master version.json
RUN git clone https://github.com/Grigoryanlab/Mosaist.git
RUN cd Mosaist && make
RUN cd ../
RUN wget https://github.com/mittinatten/freesasa/releases/download/2.0.3/freesasa-2.0.3.tar.gz && tar -zxvf freesasa-2.0.3.tar.gz
RUN cd freesasa-2.0.3 && ./configure --disable-threads --disable-xml --disable-json && make all
RUN git clone git@github.com:blacktanktop/term_peptide_design.git
