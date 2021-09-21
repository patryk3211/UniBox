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

mkdir -p $OUTPUT/shaders/gui/texture
mkdir -p $OUTPUT/shaders/gui/color
mkdir -p $OUTPUT/shaders/gui/atlas
mkdir -p $OUTPUT/shaders/gui/tooltip
mkdir -p $OUTPUT/shaders/gui/font

glslc gui/texture/texture.vert -o $OUTPUT/shaders/gui/texture/vertex.spv
glslc gui/texture/texture.frag -o $OUTPUT/shaders/gui/texture/fragment.spv

glslc gui/color/color.vert -o $OUTPUT/shaders/gui/color/vertex.spv
glslc gui/color/color.frag -o $OUTPUT/shaders/gui/color/fragment.spv

glslc gui/atlas/atlas.vert -o $OUTPUT/shaders/gui/atlas/vertex.spv
glslc gui/atlas/atlas.frag -o $OUTPUT/shaders/gui/atlas/fragment.spv

glslc gui/tooltip/tooltip.vert -o $OUTPUT/shaders/gui/tooltip/vertex.spv
glslc gui/tooltip/tooltip.frag -o $OUTPUT/shaders/gui/tooltip/fragment.spv

glslc gui/font/font.vert -o $OUTPUT/shaders/gui/font/vertex.spv
glslc gui/font/font.frag -o $OUTPUT/shaders/gui/font/fragment.spv
