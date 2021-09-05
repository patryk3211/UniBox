
typedef struct {
    ushort type;
    ushort stype;
    float paintColor[4];
    int data[4];
    float temperature;
    float velocity[3];
    float position[3];
    uint state;
} Particle;

typedef struct {
    float4 vertex;
    float4 color;
} Vertex;

typedef struct {
    float4 materialColor;
} ParticleInfo;

float3 blend(float3 a, float3 b, float blendAmount) {
    return a*(1.0f-blendAmount)+b*blendAmount;
}

#pragma PARTICLE_TYPES

#pragma PARTICLE_CODE

__kernel void generate(global Particle* particles, global Vertex* output, constant ParticleInfo* particleInfo, uint particleCount) {
    uint index = get_global_id(0);
    if(index >= particleCount) return;

    float w = -1.0;
    Particle particle = particles[index];

    if(particle.type > 0) {
        float4 baseColor = particleInfo[particle.type-1].materialColor;
        switch(particle.type) {
            #pragma PARTICLE_SWITCH
            default: break;
        }
        w = 1.0;
        for(int i = 0; i < 6; i++) 
            output[index*6+i].color = (float4)(blend(
                    baseColor.xyz,
                    (float3)(particle.paintColor[0], particle.paintColor[1], particle.paintColor[2]),
                    particle.paintColor[3]),
                baseColor.w);
    }

    const float3 offset = (float3)((uint)particle.position[0], (uint)particle.position[1], 0);
    const float4 v0 = (float4)((float3)(-0.5f, -0.5f, 0.0f)+offset, w);
    const float4 v1 = (float4)((float3)(-0.5f,  0.5f, 0.0f)+offset, w);
    const float4 v2 = (float4)((float3)( 0.5f, -0.5f, 0.0f)+offset, w);
    const float4 v3 = (float4)((float3)( 0.5f,  0.5f, 0.0f)+offset, w);

    output[index*6+0].vertex = v0;
    output[index*6+1].vertex = v1;
    output[index*6+2].vertex = v2;
    output[index*6+3].vertex = v2;
    output[index*6+4].vertex = v1;
    output[index*6+5].vertex = v3;
}