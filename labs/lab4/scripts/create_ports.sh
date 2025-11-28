#!/bin/bash

echo "Creating virtual serial ports for Linux..."

pkill -f "socat.*ttyV"

socat -d -d pty,raw,echo=0,link=/tmp/ttyV0 pty,raw,echo=0,link=/tmp/ttyV1 &

sleep 2

if [ -e "/tmp/ttyV0" ] && [ -e "/tmp/ttyV1" ]; then
    echo "Virtual ports created successfully:"
    echo "  /tmp/ttyV0 -> $(readlink /tmp/ttyV0)"
    echo "  /tmp/ttyV1 -> $(readlink /tmp/ttyV1)"
    echo ""
    echo "To test connection:"
    echo "  Terminal 1: cat /tmp/ttyV0"
    echo "  Terminal 2: echo 'Hello' > /tmp/ttyV1"
else
    echo "Failed to create virtual ports"
    exit 1
fi

echo $! > /tmp/virtual_ports.pid