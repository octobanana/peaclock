cmake_minimum_required (VERSION 3.8 FATAL_ERROR)

set (TARGET "peaclock")
project (${TARGET})

find_program (CCACHE_FOUND ccache)
if (CCACHE_FOUND)
  message ("ccache found")
  set_property (GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property (GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (CMAKE_CXX_FLAGS "-fdiagnostics-color=auto -DOB_BELLE_CONFIG_SERVER_OFF")
set (CMAKE_C_FLAGS "-fdiagnostics-color=auto -DOB_BELLE_CONFIG_SERVER_OFF")

set (DEBUG_FLAGS "-Wpedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default -Wundef -Wno-unused -std=c++17 -g")
set (DEBUG_LINK_FLAGS "-fprofile-arcs -ftest-coverage -flto")

set (RELEASE_FLAGS "-std=c++17 -s -O3")
set (RELEASE_LINK_FLAGS "-flto")

set (CMAKE_CXX_FLAGS_DEBUG ${DEBUG_FLAGS})
set (CMAKE_C_FLAGS_DEBUG ${DEBUG_FLAGS})
set (CMAKE_EXE_LINKER_FLAGS_DEBUG ${DEBUG_LINK_FLAGS})

set (CMAKE_CXX_FLAGS_RELEASE ${RELEASE_FLAGS})
set (CMAKE_C_FLAGS_RELEASE ${RELEASE_FLAGS})
set (CMAKE_EXE_LINKER_FLAGS_RELEASE ${RELEASE_LINK_FLAGS})

message ("CMAKE_BUILD_TYPE is ${CMAKE_BUILD_TYPE}")

include_directories(
  ./src
)

set (SOURCES
  src/main.cc
  src/ob/string.cc
  src/peaclock/tui.cc
  src/peaclock/readline.cc
  src/peaclock/peaclock.cc
)

set (HEADERS
)

add_executable (
  ${TARGET}
  ${SOURCES}
  ${HEADERS}
)

target_link_libraries (
  ${TARGET}
)

install (TARGETS ${TARGET} DESTINATION bin)
