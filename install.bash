#!/usr/bin/bash

## Set up the base Ubuntu image
sudo apt -y update
sudo apt -y upgrade
sudo apt install -y build-essential

# Install dependencies for Maki's Clang frontend
sudo apt install -y clang-14
sudo apt install -y cmake
sudo apt install -y libclang-14-dev
sudo apt install -y llvm-14

# Install dependencies for Maki's Python scripts
sudo apt install -y software-properties-common
sudo add-apt-repository ppa:deadsnakes/ppa
sudo apt install -y python3.10 python3-pip
sudo python3 -m pip install -U numpy
sudo python3 -m pip install -U scan-build

# Install dependencies for evaluation programs
sudo apt install -y autoconf
sudo apt install -y ed
sudo apt install -y gnutls-bin
sudo apt install -y help2man
sudo apt install -y libgif-dev
sudo apt install -y libjpeg-dev
sudo apt install -y libmotif-dev
sudo apt install -y libpng-dev
sudo apt install -y libtiff-dev
sudo apt install -y libx11-dev
sudo apt install -y libxmu-dev
sudo apt install -y libxmu-headers
sudo apt install -y libxpm-dev
sudo apt install -y texinfo
sudo apt install -y unzip
sudo apt install -y wget
sudo apt install -y xaw3dg-dev

mkdir build
cd build
cmake ..