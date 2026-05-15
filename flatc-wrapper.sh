#!/bin/bash
# Wrapper script to patch FlatBuffers version check after generation
/usr/bin/flatc "$@"
if [[ "$@" == *"WireTypes.fbs"* ]]; then
    sed -i '11,13s/^/\/\//' trade_server/protocol/WireTypes_generated.h
fi
