#!/usr/bin/env bash
sysctl -w vm.pageout_update_period=10
date
echo "./memorystress 1500 1000000 5"
./memorystress 1500 1000000 5
date
