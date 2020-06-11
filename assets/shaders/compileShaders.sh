#!/bin/bash
# rm -rf ./bin/
mkdir -pv ./bin/

for s in ./src/*
do
    file_extension="${s##*.}"
    if [ ${file_extension} = "glsl" ]; then
        continue
    fi
    d=bin/$(basename ${s})
    # src_time=$(date +%s -r ${s})
    # bin_time=$(date +%s -r ${d})
    # delta_time=$((bin_time - src_time))
    # if [ ${delta_time} -gt 0 ]; then
    #     continue
    # fi
    glslangValidator -G ${s} -o ${d}
done
