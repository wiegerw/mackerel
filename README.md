# mackerel

This module contains some extensions to the [mCRL2](http://mcrl2.org) toolset.

## Features

* Two build systems are supported: [boost.build](https://www.boost.org/build/) and CMake
* The code is C++17 based

## Tools

# mcrl32lps
Linearizes process specifications using the existing mcrl22lps algorithm.

* Supports structured sort update syntax

# mcrl3linearize
Linearizes process specifications using a new, simple linearization algorithm.
The philosophy is that it should do linearization only, and nothing else.
N.B. Currently only a very limited class of process specifications is supported.

* Supports structured sort update syntax
* Supports unguarded process specifications
* Has a much better performance than mcrl32lps on generated specifications from industrial models

## Build

The Ubuntu build is documented using a docker file [Dockerfile](build/docker/ubuntu/Dockerfile).
It contains all the commands to build and install the tools using CMake. To run the docker
itself, call `docker build -t mackerel .` in the folder containing the Dockerfile.
