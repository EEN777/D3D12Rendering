#include "Common.hlsl"

RaytracingAccelerationStructure SceneBVH : register(t0, space3);
Texture2D<float4> texture[] : register(t2, space0);
ByteAddressBuffer vertices[] : register(t0, space1);
ByteAddressBuffer indices[] : register(t0, space2);

cbuffer PointLightLocation : register(b0)
{
    float3 lightPosition;
};

cbuffer FrameInfo : register(b1)
{
    uint frameNumber;
};

uint maxDepth = 4;

[shader("miss")]
void IndirectMiss(inout IndirectHitInfo payload : SV_RayPayload)
{
    payload.color = float3(0.0f, 0.0f, 0.0f);
}


uint3 GetIndices(uint triangleIndex)
{
    uint baseIndex = (triangleIndex * 3);
    int address = (baseIndex * 4);
    return indices[InstanceID()].Load3(address);
}

VertexAttributes GetVertexAttributes(uint triangleIndex, float3 barycentrics)
{
    uint3 indices = GetIndices(triangleIndex);
    VertexAttributes v;
    v.position = float3(0, 0, 0);
    v.uv = float2(0, 0);

    for (uint i = 0; i < 3; i++)
    {
        int address = (indices[i] * 6) * 4;
        v.position += asfloat(vertices[InstanceID()].Load3(address)) * barycentrics[i];
        address += (3 * 4);
        v.uv += asfloat(vertices[InstanceID()].Load2(address)) * barycentrics[i];
    }

    return v;
}

[shader("closesthit")]
void IndirectHit(inout IndirectHitInfo payload, Attributes attrib)
{
    uint triangleIndex = PrimitiveIndex();
    float3 barycentrics = float3((1.0f - attrib.bary.x - attrib.bary.y), attrib.bary.x, attrib.bary.y);
    VertexAttributes vertex = GetVertexAttributes(triangleIndex, barycentrics);

    uint colorMapIndex = InstanceID() * 2;
    uint normalMapIndex = colorMapIndex + 1;
        
    uint width, height;
    texture[colorMapIndex].GetDimensions(width, height);
    
    int2 coord = int2(vertex.uv.x * width, vertex.uv.y * height);
    float3 color = texture[colorMapIndex].Load(int3(coord, 0)).rgb;
        
    texture[normalMapIndex].GetDimensions(width, height);
    coord = int2(vertex.uv.x * width, vertex.uv.y * height);
    float3 normal = texture[normalMapIndex].Load(int3(coord, 0)).rgb;
        
    float3 lightPos = float3(lightPosition);
    
    float3 worldOrigin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();

    float3 lightDir = normalize(lightPos - worldOrigin);
    normal = normalize(normal);
    normal = normal * -1;
    
    float ambientLightIntensity = 0.01f;
        
    float NdotL = saturate(dot(normal, lightDir));
        
    float dist = length(worldOrigin - lightPos);
        
    float radius = 2000;
    
    payload.color *= color;
    payload.currentDepth += 1;
    
    float3 u, v, w;
    w = normal;
    u = normalize(cross((abs(w.x) > 0.1 ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f)), w));
    v = cross(w, u);
        
    uint state = RNG::SeedThread(payload.currentDepth * frameNumber * payload.pixelCoords.x * payload.pixelCoords.y);
    float randX = RNG::Random01(state);
    state += 1;
    float randY = RNG::Random01(state);
    float3 randomPoint = SampleCosineHemisphere(float2(randX, randY));
    randomPoint = randomPoint.x * u + randomPoint.y * v + randomPoint.z * w;
        
    float3 newDir = normalize(randomPoint - worldOrigin);
    
    worldOrigin += normal * 0.001f;
    
    RayDesc ray;
    ray.Origin = worldOrigin;
    ray.Direction = newDir;
    ray.TMin = 0.01;
    ray.TMax = 100000;
    
    if (payload.currentDepth < maxDepth)
    {
        TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 2, 0, 2, ray, payload);
    }

}