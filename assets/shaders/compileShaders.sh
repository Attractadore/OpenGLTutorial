#!/bin/bash

for f in ./src/*
do
    glslangValidator -G ${f} -o bin/$(basename ${f})
done