FROM ubuntu:16.04

RUN apt update && apt install -y libpcl-dev git cmake libproj-dev python-vtk6
RUN ln -s /usr/lib/x86_64-linux-gnu/libvtkCommonCore-6.2.so /usr/lib/libvtkproj4.so

RUN mkdir /root/build
RUN mkdir /root/results

WORKDIR /root

ADD datasets /root/datasets
ADD include /root/include
ADD src /root/src
ADD scripts /root/scripts
ADD CMakeLists.txt /root/CMakeLists.txt

WORKDIR /root/build
RUN cmake ..
RUN make -j4; exit 0
CMD tail -f /dev/null
