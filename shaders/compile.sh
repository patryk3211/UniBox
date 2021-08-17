#!/bin/bash

ROOT="/home/patryk/Projects/UniBox/shaders"

OUTPUT="../run"

cd $ROOT

mkdir -p $OUTPUT/shaders/default
mkdir -p $OUTPUT/shaders/compute
glslc default/default.vert -o $OUTPUT/shaders/default/vertex.spv
glslc default/default.frag -o $OUTPUT/shaders/default/fragment.spv
glslc compute/meshGenerator.comp -o $OUTPUT/shaders/compute/meshGen.spv
glslc compute/simulator.comp -o $OUTPUT/shaders/compute/simulator.spv
glslc compute/gridBuilder.comp -o $OUTPUT/shaders/compute/gridBuilder.spv
glslc compute/gridResetter.comp -o $OUTPUT/shaders/compute/gridResetter.spv