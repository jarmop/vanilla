#!/bin/bash

rm -f build/bin/VanillaVulkan 

time cmake -B build
time cmake --build build

build/bin/VanillaVulkan 