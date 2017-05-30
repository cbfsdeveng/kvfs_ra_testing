#!/usr/bin/env bash
# update repo information
set -x
sudo apt-get update

# basic s/w build tools
sudo apt-get install make automake autoconf
sudo apt-get install gcc g++ libtool golang
sudo apt-get install python-pip
pip install --upgrade pip

# libraries
sudo apt-get install libprotobuf-dev protobuf-compiler
sudo apt-get install libgoogle-glog-dev
sudo apt-get install fio
sudo apt-get install libfuse-dev
sudo apt-get install libboost-all-dev
sudo apt-get install libgoogle-perftools-dev
sudo apt-get install libsnappy-dev
sudo apt-get install s3cmd

# personal choice development tools
sudo apt-get install gdb
sudo apt-get install google-perftools
