#!/usr/bin/env bash
set -e

BUILD_TYPE="release"

if [[ $# > 0 ]]; then
  if [[ $1 == "--debug" ]]; then
    BUILD_TYPE="debug"
  elif [[ $1 == "--release" ]]; then
    BUILD_TYPE="release"
  else
    printf "usage: ./install.sh [--debug|--release]\n";
    exit 1
  fi
fi

# source environment variables
source ./env.sh

# build
./build.sh --${BUILD_TYPE}
cd build/${BUILD_TYPE}

# install
printf "\nInstalling ${APP} in ${BUILD_TYPE} mode\n"
if command -v sudo > /dev/null; then
  sudo make install
elif command -v doas > /dev/null; then
  doas make install
else
  printf "Don't know how to elevate privileges, bailing.\n"
  exit 1
fi
