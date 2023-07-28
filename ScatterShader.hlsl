#include "Common.hlsl"

struct ShadowHitInfo
{
    bool isHit;
};

struct STriVertex
{
    float3 vertex;
    float3 TexCoord;
};

struct ScatterHitInfo
{
    uint maxDepth;
    uint currentDepth;
    float3 cumulativeColor;
    uint2 pixelCoords;
    bool hasHitLight;
};


RaytracingAccelerationStructure SceneBVH : register(t0, space3);
Texture2D<float4> texture[] : register(t2, space0);
ByteAddressBuffer vertices[] : register(t0, space1);
ByteAddressBuffer indices[] : register(t0, space2);
StructuredBuffer<MaterialProperties> materialProperties[] : register(t0, space4);

cbuffer PointLightLocation : register(b0)
{
    float3 lightPosition;
    float3 lightColor;
    float lightIntensity;
};

cbuffer FrameInfo : register(b1)
{
    uint frameNumber;
};

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
void ScatterClosestHit(inout ScatterHitInfo payload, Attributes attrib)
{
    MaterialProperties currentMaterialProperties = materialProperties[InstanceID()].Load(0);
    
    if (currentMaterialProperties.isLight == true)
    {
        payload.cumulativeColor += lightColor;
        payload.hasHitLight = true;
    }
    else if (payload.currentDepth <= payload.maxDepth)
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
        normal = normalize(normal);
        normal = normal * -1;
        
        
        // Find the world - space hit position
        float3 worldOrigin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
        
        float distToLight = length(lightPosition - worldOrigin);
        float3 toLight = lightPosition - worldOrigin;
        toLight = normalize(toLight);
        
        float NDotL = saturate(dot(normal, toLight));
        
        payload.cumulativeColor += NDotL * (lightColor * lightIntensity) * color.rgb / PI;
        
        payload.currentDepth += 1;
        
        uint state = RNG::SeedThread(frameNumber * payload.pixelCoords.x * payload.pixelCoords.y);
        float randX = RNG::Random01(state);
        state += 1;
        float randY = RNG::Random01(state);
        float3 randomDir = SampleCosineHemisphere(float2(randX, randY));
        
        float3 u, v, w;
        w = normal;
        u = normalize(cross((abs(w.x) > 0.1 ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f)), w));
        v = cross(w, u);
        
        randomDir = randomDir.x * u + randomDir.y * v + randomDir.z * w;

        NDotL = saturate(dot(normal, randomDir));
        
        worldOrigin += normal * 0.01f;
        
        RayDesc scatterRay;
        scatterRay.Origin = worldOrigin;
        scatterRay.Direction = randomDir;
        scatterRay.TMin = 0.01;
        scatterRay.TMax = 100000;
        
        TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 2, 0, 2, scatterRay, payload);
    }
}

[shader("miss")]
void ScatterMiss(inout ScatterHitInfo payload : SV_RayPayload)
{
    payload.cumulativeColor = float3(0.0f, 0.0f, 0.0f);
}