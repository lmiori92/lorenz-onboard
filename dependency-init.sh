#!/bin/sh

# This script will initialize the repository by downloading or cloning dependencies.

deps_git_clone()
{
    echo "Start cloning $1"
    git clone $1
    echo "End cloning $1 with result code $?"
}

mkdir -p dependencies

cd dependencies


deps_git_clone https://github.com/dergraaf/avr-can-lib

