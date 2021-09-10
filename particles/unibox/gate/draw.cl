float3 unibox_gate_draw(float3 base, const Particle vertex) {
    if(vertex.data[1] > 0) return (float3)(105/256.0, 127/256.0, 148/256.0);
    else return base;
}