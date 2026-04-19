#!/bin/bash

rm -f build/bin/HowToVulkan 

time cmake -B build
time cmake --build build

build/bin/HowToVulkan 