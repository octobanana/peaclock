#!/usr/bin/env bash
:<<'####'
                                    88888888
                                  888888888888
                                 88888888888888
                                8888888888888888
                               888888888888888888
                              888888  8888  888888
                              88888    88    88888
                              888888  8888  888888
                              88888888888888888888
                              88888888888888888888
                             8888888888888888888888
                          8888888888888888888888888888
                        88888888888888888888888888888888
                              88888888888888888888
                            888888888888888888888888
                           888888  8888888888  888888
                           888     8888  8888     888
                                   888    888

                                   OCTOBANANA

Licensed under the MIT License

Copyright (c) 2019 Brett Robinson <https://octobanana.com/>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
####

set -e

SELF="${0##*/}"
SUB_COMMAND=""
BUILD_TYPE=""

usage() {
  if [[ "${SUB_COMMAND}" == "clean" ]]; then
    printf "Usage:\n"
    printf "  ./%s clean [--release|--debug]\n" "${SELF}"
  elif [[ "${SUB_COMMAND}" == "dev" ]]; then
    printf "Usage:\n"
    printf "  ./%s dev [--release|--debug] [-- <args>]\n" "${SELF}"
  elif [[ "${SUB_COMMAND}" == "build" ]]; then
    printf "Usage:\n"
    printf "  ./%s build [--release|--debug] [-- <args>]\n" "${SELF}"
  elif [[ "${SUB_COMMAND}" == "install" ]]; then
    printf "Usage:\n"
    printf "  ./%s install [--release|--debug] [-- <args>]\n" "${SELF}"
  else
    printf "Usage:\n"
    printf "  ./%s --help\n" "${SELF}"
    printf "  ./%s clean [--release|--debug]\n" "${SELF}"
    printf "  ./%s dev [--release|--debug] [-- <args>]\n" "${SELF}"
    printf "  ./%s build [--release|--debug] [-- <args>]\n" "${SELF}"
    printf "  ./%s install [--release|--debug] [-- <args>]\n" "${SELF}"
    printf "\n"
    printf "Examples:\n"
    printf "  ./%s build --release -- -DCMAKE_CXX_COMPILER='clang++'\n" "${SELF}"
    printf "  ./%s install --release\n" "${SELF}"
  fi

  if [[ "${*}" != "" ]] ; then
    printf "\nError: %s\n" "${*}"
  fi
}

is_project_root() {
  if ! [[ -e ./.gitignore ]]; then
    usage "not in project root"
    exit 1
  fi
}

on_clean() {
  while [[ "${#}" -gt 0 ]]; do case "${1}" in
    -r|--release)
      BUILD_TYPE="release"
      shift
      ;;
    -d|--debug)
      BUILD_TYPE="debug"
      shift
      ;;
    -h|--help)
      usage
      exit
      ;;
    *)
      usage "invalid argument '${1}'"
      exit 1
      ;;
    esac
  done

  do_clean "${@}"
}

do_clean() {
  rm -rf ./build/"${BUILD_TYPE}" ./.m8 ./m8
}

on_dev() {
  BUILD_TYPE="debug"

  while [[ "${#}" -gt 0 ]]; do case "${1}" in
    -r|--release)
      BUILD_TYPE="release"
      shift
      ;;
    -d|--debug)
      BUILD_TYPE="debug"
      shift
      ;;
    -h|--help)
      usage
      exit
      ;;
    --)
      shift
      break
      ;;
    *)
      usage "invalid argument '${1}'"
      exit 1
      ;;
    esac
  done

  do_dev "${@}"
}

print_status() {
  printf "\n"
  if [[ "${1}" -eq 0 ]]; then
    if [[ "${2}" -ne 0 ]]; then
      hr -b reverse -f green-bright -s 'PASS '
    else
      hr -b reverse -f green-bright -s 'PASS '
    fi
  else
    hr -b reverse -f red-bright -s 'FAIL '
  fi
  printf "\n"
}

do_dev() {
  set +e

  # log files
  log_base="./log"
  log_build="${log_base}/build.log"
  log_error="${log_base}/error.log"
  log_warning="${log_base}/warning.log"
  mkdir -p ./build/"${BUILD_TYPE}"
  cd ./build/"${BUILD_TYPE}"

  # create log directory
  mkdir -p "${log_base}"

  # clear log files
  :> "${log_build}"
  :> "${log_error}"
  :> "${log_warning}"

  cmake ../../ -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" "${@}"
  time unbuffer make -j $(( 2 * $(nproc --all) )) 2>&1 | tee "${log_build}"

  # preserve make exit status
  STATUS="${PIPESTATUS[0]}"

  # print horizontal rule
  hr

  # write warnings and errors to log files
  printf "%s" "$(grep -Ei 'warning:' ${log_build})" > "${log_warning}"
  printf "%s" "$(grep -Ei 'error:' ${log_build})" > "${log_error}"

  # get number of warnings
  NUM_WARN=$(grep -Eic 'warning:' "${log_build}")

  # print warnings
  if [[ "${NUM_WARN}" -ne 0 ]]; then
    printf "\nWarnings: %s\n" "${NUM_WARN}"
    printf "%s\n" "$(grep -Eo ' \[.*?(\-W.*?).*?\]' "${log_build}"\
      | cut -c1- | sort | uniq -c | sort -nr)"
    printf "\n"
  fi

  # print errors
  if [[ "${STATUS}" -ne 0 ]]; then
    printf "%s\n" "$(grep -Ei 'error:' ${log_build})"
    printf "\nErrors: %s\n\n" "$(grep -Eic 'error:' ${log_build})"
  fi

  # print date
  date

  # print status
  print_status "${STATUS}" "${NUM_WARN}"

  # return make exit status
  exit "${STATUS}"
}

on_build() {
  BUILD_TYPE="release"

  while [[ "${#}" -gt 0 ]]; do case "${1}" in
    -r|--release)
      BUILD_TYPE="release"
      shift
      ;;
    -d|--debug)
      BUILD_TYPE="debug"
      shift
      ;;
    -h|--help)
      usage
      exit
      ;;
    --)
      shift
      break
      ;;
    *)
      usage "invalid argument '${1}'"
      exit 1
      ;;
    esac
  done

  do_build "${@}"
}

do_build() {
  printf "Build type: %s\n" "${BUILD_TYPE}"
  mkdir -p ./build/"${BUILD_TYPE}"
  cd ./build/"${BUILD_TYPE}"
  cmake ../../ -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" "${@}"

  if [[ "${BUILD_TYPE}" == "release" ]]; then
    make -j2
  elif [[ "${BUILD_TYPE}" == "debug" ]]; then
    time make -j2
  fi

  cd ../../
}

on_install() {
  BUILD_TYPE="release"

  while [[ "${#}" -gt 0 ]]; do case "${1}" in
    -r|--release)
      BUILD_TYPE="release"
      shift
      ;;
    -d|--debug)
      BUILD_TYPE="debug"
      shift
      ;;
    -h|--help)
      usage
      exit
      ;;
    --)
      shift
      break
      ;;
    *)
      usage "invalid argument '${1}'"
      exit 1
      ;;
    esac
  done

  do_install "${@}"
}

do_install() {
  if [[ "${SUB_COMMAND}" != "build" ]]; then
    do_build "${@}"
  fi

  cd ./build/"${BUILD_TYPE}"
  set +e
  OUTPUT="$(make install 2>&1)"
  OUTPUT_STATUS="${?}"
  set -e

  if [[ "${OUTPUT_STATUS}" -ne 0 ]]; then
    if command -v sudo > /dev/null; then
      sudo make install
    elif command -v doas > /dev/null; then
      doas make install
    else
      printf "Error: unable to elevate privileges, tried 'sudo' and 'doas'\n"
      exit 1
    fi
  else
    printf "%s\n" "${OUTPUT}"
  fi

  cd ../../
}

on_normal() {
  while [[ "${#}" -gt 0 ]]; do case "${1}" in
    -h|--help)
      usage
      exit
      ;;
    *)
      usage "invalid argument '${1}'"
      exit 1
      ;;
    esac
  done
}

subcommand() {
  if [[ "${#}" -gt 0 ]]; then
    if [[ "${1}" == "c" || "${1}" == "clean" ]]; then
      SUB_COMMAND="clean"
      shift
      on_clean "${@}"
    elif [[ "${1}" == "d" || "${1}" == "dev" ]]; then
      SUB_COMMAND="dev"
      shift
      on_dev "${@}"
    elif [[ "${1}" == "b" || "${1}" == "build" ]]; then
      SUB_COMMAND="build"
      shift
      on_build "${@}"
    elif [[ "${1}" == "i" || "${1}" == "install" ]]; then
      SUB_COMMAND="install"
      shift
      on_install "${@}"
    else
      on_normal "${@}"
    fi
  else
    usage "expected subcommand and/or arguments"
    exit 1
  fi
}

main() {
  is_project_root
  subcommand "${@}"
}

main "${@}"
