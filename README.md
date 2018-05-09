# mackerel

The mackerel library is derived from the [mCRL2](http://mcrl2.org) toolset. It is a test bed for
language extensions and alternative implementations of existing algorithms.

## Rationale

The work on the mCRL2 toolset is for a large part performance driven. This makes it very difficult
to replace some of the heavily optimized but undocumented implementations by alternative versions.
Moreover, in the mCRL2 toolset there is no room for experimentation with new language features.

The mackerel library contains a subset of the mCRL2 library. The goals of mackerel are
to experiment with new language features, and to replace undocumented implementations with well
specified ones that have proper interfaces. Given the amount of undocumented code this is a long
term project. But for the future this is absolutely necessary, since the undocumented code blocks
any kind of progress, and it takes too much time to maintain.

## Roadmap

Some of the major goals are:

* Make a new implementation of the data type checker
* Make a new implementation of the rewriter
* Make a new implementation of the linearization of process specifications
* Make a new implementation of the state space generation

Some of the minor goals are:

* Add multi actions to the process library
* Properly handle sort normalization. This should help to simplify the class data_specification

## Tools

### mcrl32lps
Linearizes process specifications using the existing mcrl22lps algorithm.

* Supports structured sort update syntax

### mcrl3linearize
Linearizes process specifications using a new, simple linearization algorithm.
The philosophy is that it should do linearization only, and nothing else.
N.B. Currently only a very limited class of process specifications is supported.

* Supports structured sort update syntax
* Supports unguarded process specifications
* Has a much better performance than mcrl32lps on generated specifications from industrial models

## Build

Two build systems are supported: [boost.build](https://www.boost.org/build/) and CMake.

The code is C++14 based, but C++17 is allowed as well.

The Ubuntu build is documented using a docker file [Dockerfile](build/docker/ubuntu/Dockerfile).
It contains all the commands to build and install the tools using CMake. To run the docker
itself, call `docker build -t mackerel .` in the folder containing the Dockerfile.
