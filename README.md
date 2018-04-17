# mackerel

This module contains some extensions to the [mCRL2](http://mcrl2.org) toolset.

## Features

* Two build systems are supported: [boost.build](https://www.boost.org/build/) and CMake
* The code is C++17 based

## Extensions

* Process specifications may be unguarded
* Structured sorts have update syntax (finished)
* There is a new linearization tool mcrl3linearize (work in progress)

## Build

The Ubuntu build is documented using a docker file [Dockerfile](build/docker/ubuntu/Dockerfile).
It contains all the commands to build and install the tools using CMake. To run the docker
itself, call `docker build -t mackerel .` in the folder containing the Dockerfile.
