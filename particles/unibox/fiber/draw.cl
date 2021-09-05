float3 draw(float3 base, const Particle vertex) {
    if(vertex.data[0] > 0) {
        const uint red = (vertex.data[1]>>24)&0xFF;
        const uint yel = (vertex.data[1]>>16)&0xF0;
        const uint grn = (vertex.data[1]>>12)&0xFF;
        const uint cya = (vertex.data[1]>>4)&0xF0;
        const uint blu = (vertex.data[1])&0xFF;
        float3 lightColor = (float3)((red+(yel/2.0))/375.0, (grn+(yel/2.0)+(cya/2.0))/495.0, (blu+(cya/2.0))/375.0);
        return mix(base, lightColor, (float)(vertex.data[0]/2.0));
    }
    return base;
}