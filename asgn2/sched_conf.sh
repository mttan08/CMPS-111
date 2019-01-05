#!/usr/bin/env bash

if [ "$(id -u)" != 0 ]; then
    echo "Must be run as root"
    exit 1
fi

kenv -qv SCHED_CONFIG_EV=$1
