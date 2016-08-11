# Entity sytem

It is c++ entity system (c++14)
* only header
* without rtti
* without exception

## Hello World

TODO

## Required

* c++ compiler with support c++14
* [cmake](https://cmake.org) 3.0 or higher
* [boost](http://boost.org) unit test (not mandatory, to run unit test)
* [allegro](http://allegro.org) 5.2 or higher (not mandatory, to run demos)

## Build (Unix)

By default :
* unit test is builded (if there are boost unit test)
* in debug mode

Common cmake option (to add on common line) :

 Option | Value | Default | Description
--------| ------|---------|------------
CMAKE_BUILD_TYPE | Debug or Release | Debug | Select build type
CMAKE_INSTALL_PREFIX | path | /usr/local | Prefix installation
DISABLE_UNITTEST | ON or OFF | OFF | Disable unittest
DISABLE_DEMOS | ON or OFF | OFF | Disable demos

run cmake :
```shell
luacxx $ mkdir build && cd build
build  $ cmake ..
```

build :
```shell
build $ make
```

run unit test :
```shell
build $ make test
```

install :
```shell
build $ make install
```

## Licence

cf [Boost Licence](http://www.boost.org/LICENSE_1_0.txt)
