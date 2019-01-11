# Peaclock
A colourful binary clock for the terminal.

![default](https://raw.githubusercontent.com/octobanana/peaclock/master/assets/peaclock.png)
![binary](https://raw.githubusercontent.com/octobanana/peaclock/master/assets/binary.png)
![digital](https://raw.githubusercontent.com/octobanana/peaclock/master/assets/digital.png)

## About
Peaclock is a customizable binary clock made for the terminal.

### Features
* digital clock
* binary clock
* 12/24 hour time
* customize with hex colour codes
* set a custom character for the binary clock graphic
* compact or expanded mode
* toggle visibility of digital and binary clocks
* command prompt with a readline-like interface

## Build
### Environment
* Linux (supported)
* BSD (untested)
* macOS (untested)

### Requirements
* C++17 compiler
* CMake >= 3.8

### Dependencies
* none

### Libraries:
* [parg](https://github.com/octobanana/parg): for parsing CLI args, included as `./src/ob/parg.hh`

The following shell command will build the project in release mode:
```sh
./build.sh
```
To build in debug mode, run the script with the `--debug` flag.

## Install
The following shell command will install the project in release mode:
```sh
./install.sh
```
To install in debug mode, run the script with the `--debug` flag.

## License
This project is licensed under the MIT License.

Copyright (c) 2019 [Brett Robinson](https://octobanana.com/)

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
