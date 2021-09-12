#!/bin/bash

ROOT="/home/patryk/Projects/UniBox/shaders"

OUTPUT="../run"

cd $ROOT

mkdir -p $OUTPUT/shaders/default
mkdir -p $OUTPUT/shaders/compute
glslc default/default.vert -o $OUTPUT/shaders/default/vertex.spv
glslc default/default.frag -o $OUTPUT/shaders/default/fragment.spv
cp compute/meshGenerator.cl $OUTPUT/shaders/compute/meshGenerator.cl
cp compute/simulator.cl $OUTPUT/shaders/compute/simulator.cl

mkdir -p $OUTPUT/shaders/gui/default
glslc gui/default/default.vert -o $OUTPUT/shaders/gui/default/vertex.spv
glslc gui/default/default.frag -o $OUTPUT/shaders/gui/default/fragment.spv
