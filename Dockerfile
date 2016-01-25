FROM ubuntu:14.04

RUN apt-get update
RUN apt-get install -y software-properties-common libav-tools python-dev libfreetype6-dev libpng-dev pkg-config pandoc cmake g++ libboost-dev libgsl0-dev libhdf5-serial-dev libboost-regex-dev python python-numpy python-scipy python-pip python-zmq
RUN add-apt-repository ppa:mc3man/trusty-media -y; apt-get update; apt-get install -y ffmpeg

RUN pip install cython jupyter matplotlib
ADD . /usr/src/ecell4

RUN cd /usr/src/ecell4; export PREFIX=/usr/local; export PYTHONPATH=/usr/local/lib/python2.7/site-packages:$PYTHONPATH; ./install.sh --python2 --hdf5

EXPOSE 8888
CMD LD_LIBRARY_PATH=/usr/local/lib jupyter-notebook --notebook-dir='/usr/src/ecell4/ipynb' --no-browser --ip='*' --port 8888
