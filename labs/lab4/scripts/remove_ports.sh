#!/bin/bash

echo "Removing virtual serial ports..."

if [ -f "/tmp/virtual_ports.pid" ]; then
    kill $(cat /tmp/virtual_ports.pid)
    rm /tmp/virtual_ports.pid
fi

rm -f /tmp/ttyV0 /tmp/ttyV1

echo "Virtual ports removed"