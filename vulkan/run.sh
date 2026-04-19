#!/bin/bash

rm build/bin/HowToVulkan 

time cmake -B build
time cmake --build build

build/bin/HowToVulkan 