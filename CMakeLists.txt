#[[
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
]]

cmake_minimum_required (VERSION 3.8 FATAL_ERROR)

set (OB_TARGET "peaclock")
set (OB_VERSION "0.4.2")
set (OB_SOURCES
  src/main.cc
  src/ob/string.cc
  src/ob/readline.cc
  src/peaclock/tui.cc
  src/peaclock/peaclock.cc
)
set (OB_LINK_LIBRARIES
  ${OB_LINK_LIBRARIES}
  pthread
  icuuc
  icui18n
)
set (OB_INCLUDE_DIRECTORIES
  ./src
)
# set (CMAKE_CXX_COMPILER "clang++")

project (${OB_TARGET} VERSION ${OB_VERSION} LANGUAGES CXX)

message ("-- Building ${OB_TARGET} ${OB_VERSION} in ${CMAKE_BUILD_TYPE} mode:")
message ("     System: ${CMAKE_SYSTEM}")
message ("     CXX compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

set (OB_COMPILER_VERSION_GNU "8.0.0")
set (OB_COMPILER_VERSION_CLANG "7.0.0")
set (OB_COMPILER_VERSION_APPLECLANG "11.0.0")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "${OB_COMPILER_VERSION_GNU}")
    message (FATAL_ERROR ">> The ${CMAKE_CXX_COMPILER_ID} compiler version ${CMAKE_CXX_COMPILER_VERSION} is unsupported, requires at least version ${OB_COMPILER_VERSION_GNU}")
  endif ()

  set (OB_FLAGS_GENERAL "-std=c++17 -fdiagnostics-color=auto -fmax-errors=8")

  set (OB_FLAGS_DEBUG "-g -Og -rdynamic -Wpedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default -Wundef -Wno-unused")
  set (DEBUG_LINK_FLAGS "-fprofile-arcs -ftest-coverage")

  set (OB_FLAGS_RELEASE "-O3 -flto")
  set (OB_LINKER_FLAGS_RELEASE "-O3 -flto -s")
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "${OB_COMPILER_VERSION_CLANG}")
      message (FATAL_ERROR ">> The ${CMAKE_CXX_COMPILER_ID} compiler version ${CMAKE_CXX_COMPILER_VERSION} is unsupported, requires at least version ${OB_COMPILER_VERSION_CLANG}")
    endif ()
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    message (FATAL_ERROR ">> The ${CMAKE_CXX_COMPILER_ID} compiler is unsupported")
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "${OB_COMPILER_VERSION_APPLECLANG}")
      message (FATAL_ERROR ">> The ${CMAKE_CXX_COMPILER_ID} compiler version ${CMAKE_CXX_COMPILER_VERSION} is unsupported, requires at least version ${OB_COMPILER_VERSION_APPLECLANG}")
    endif ()
  else ()
    message (FATAL_ERROR ">> The ${CMAKE_CXX_COMPILER_ID} compiler is unsupported")
  endif ()

  set (OB_FLAGS_GENERAL "-std=c++17 -fdiagnostics-color=auto -ferror-limit=8 -stdlib=libc++")

  set (OB_FLAGS_DEBUG "-g -Og -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-disabled-macro-expansion -Wno-exit-time-destructors -Wno-global-constructors -Wno-weak-vtables -Wno-return-std-move-in-c++11 -Wno-c++11-narrowing")
  set (OB_LINKER_FLAGS_DEBUG "-fprofile-arcs -ftest-coverage")

  set (OB_FLAGS_RELEASE "-O3 -flto")
  set (OB_LINKER_FLAGS_RELEASE "-O3 -flto -s")
else ()
  message (FATAL_ERROR ">> The ${CMAKE_CXX_COMPILER_ID} compiler is unsupported")
endif ()

if ("${CMAKE_BUILD_TYPE}" STREQUAL "debug")
  find_program (OB_CCACHE_FOUND ccache)
  if (OB_CCACHE_FOUND)
    message ("-- Found ccache")
    set_property (GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property (GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
  endif ()
endif ()

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OB_FLAGS_GENERAL}")

set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${OB_FLAGS_DEBUG} -DDEBUG")
set (CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${OB_LINKER_FLAGS_DEBUG}")

set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${OB_FLAGS_RELEASE} -DNDEBUG")
set (CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} ${OB_LINKER_FLAGS_RELEASE}")

add_executable (
  ${OB_TARGET}
  ${OB_SOURCES}
)

target_include_directories (
  ${OB_TARGET}
  PRIVATE
  ${OB_INCLUDE_DIRECTORIES}
)

target_link_libraries (${OB_TARGET}
  ${OB_LINK_LIBRARIES}
)

install (TARGETS ${OB_TARGET} DESTINATION bin)
