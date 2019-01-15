#!/usr/bin/env bash
set -e

BUILD_TYPE="release"

if [[ $# > 0 ]]; then
  if [[ $1 == "--debug" ]]; then
    BUILD_TYPE="debug"
  elif [[ $1 == "--release" ]]; then
    BUILD_TYPE="release"
  else
    printf "usage: ./build.sh [--debug|--release]\n";
    exit 1
  fi
fi

# source environment variables
source ./env.sh

printf "\nBuilding ${APP} in ${BUILD_TYPE} mode\n"

printf "\nCompiling ${APP}\n"
mkdir -p build/${BUILD_TYPE}
cd build/${BUILD_TYPE}
cmake ../../ -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
time make
