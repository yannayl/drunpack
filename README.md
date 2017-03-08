# DrUnpack

DynamoRio based simple & generic unpacker.

DrUnpack inspects the memory of process during execution and dump _suspicious_ memory regious.
Executed memory is considered suspicious if it's writable or not mapped to any excutable (also libraries) file.

## Requirements

* [cmake][https://cmake.org/download]
* make
* [DynamoRio][https://github.com/DynamoRIO/dynamorio/wiki/Downloads]

## Build

### Linux

```bash
cd $PROJECT
mkdir build
cd build
cmake -DDynamoRIO_DIR=$DYNAMORIO_HOME/cmake ..
make
```

## Usage

```drrun -c $PROJECT/build/bin/libunpack.so -- /path/to/binary```
The client creates dump for each suspicious memory being executed.

## Platforms

Theoretically, should support all platforms [supported by DynamoRio][https://github.com/DynamoRIO/dynamorio/blob/master/README.md#about-dynamorio].

Practically, it's been tested on:
* Linux (ubuntu)
