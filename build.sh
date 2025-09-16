#!/bin/bash

# Setup QNX environment
source $HOME/qnx800/qnxsdp-env.sh

# Go to project directory
cd $HOME/qnxprojects/my-project

# Run make with the real rules for ARM64 (aarch64) target
make all PLATFORM=aarch64le
