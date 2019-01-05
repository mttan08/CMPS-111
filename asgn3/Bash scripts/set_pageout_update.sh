#!/usr/bin/env bash
sysctl -w vm.pageout_update_period=$1
