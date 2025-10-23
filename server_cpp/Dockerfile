FROM ubuntu:latest

# USER root
RUN apt-get update && apt-get install -y cmake build-essential git
WORKDIR /deps

RUN git clone --recurse-submodules -b v1.74.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc

# WORKDIR /app/grpc/cmake/build
# RUN cmake -DgRPC_INSTALL=ON \
#       -DgRPC_BUILD_TESTS=OFF \
#       -DCMAKE_CXX_STANDARD=17 \
#       -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR \
#       ../..
# RUN make -j 4
# RUN make install
# WORKDIR /app
RUN mkdir -p /deps/grpc/build && cd /deps/grpc/build && \
    cmake -DgRPC_INSTALL=ON \
    -DgRPC_BUILD_TESTS=OFF \
    .. && \
    make -j4 install