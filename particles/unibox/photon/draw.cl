float3 unibox_photon_draw(float3 base, const Particle vertex) {
    const uint red = (vertex.data[0]>>24)&0xFF;
    const uint yel = (vertex.data[0]>>16)&0xF0;
    const uint grn = (vertex.data[0]>>12)&0xFF;
    const uint cya = (vertex.data[0]>>4)&0xF0;
    const uint blu = (vertex.data[0])&0xFF;
    float3 lightColor = (float3)((red+(yel/2.0))/375.0, (grn+(yel/2.0)+(cya/2.0))/495.0, (blu+(cya/2.0))/375.0);
    return lightColor;
}