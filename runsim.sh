#!/bin/bash

SIM="./devel/sim"
MAKEFILE_DIR="./devel"

if [[ "$1" == "-c" ]]; then
    echo "Building simulator..."
    (cd "$MAKEFILE_DIR" && make)
    exit $?
fi

if [[ ! -f "$SIM" ]]; then
    echo "Simulator not found. Build it first with ./runsim.sh -c"
    exit 1
fi

"$SIM"