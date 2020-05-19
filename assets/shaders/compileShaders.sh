#!/bin/bash

for f in ./src/*
do
    echo ${f}
    glslangValidator -G ${f} -o bin/$(basename ${f})
done