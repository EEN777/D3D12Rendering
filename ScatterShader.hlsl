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
};


RaytracingAccelerationStructure SceneBVH : register(t0, space3);
Texture2D<float4> texture[] : register(t2, space0);
ByteAddressBuffer vertices[] : register(t0, space1);
ByteAddressBuffer indices[] : register(t0, space2);

cbuffer PointLightLocation : register(b0)
{
    float3 lightPosition;
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
    if (payload.currentDepth != payload.maxDepth)
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
        
        float NdotL = max(dot(normal, lightDir), 0.0f);
        
        float radius = 2000;
        
        float3 lightColor = float3(1.0f, 1.0f, 1.0f);
        //float lightIntensity = 100;
        //lightColor *= lightIntensity;
        
        float3 baseColor = color.rgb;
        
        float dist = length(worldOrigin - lightPos);
        
        if (dist > radius)
        {
            baseColor = baseColor * ambientLightIntensity;
        }
        else
        {
            float ratio = max((1.0f - (dist / radius)), 0.0f);
            baseColor = baseColor * (ratio * lightColor * NdotL + ambientLightIntensity);
        }
    
        float3 u, v, w;
        w = normal;
        u = normalize(cross((abs(w.x) > 0.1 ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f)), w));
        v = cross(w, u);
        
        uint state = RNG::SeedThread(777 * payload.currentDepth);
        float randX = RNG::Random01(state);
        state = RNG::SeedThread(322 * payload.currentDepth);
        float randY = RNG::Random01(state);
        float3 randomPoint = SampleCosineHemisphere(float2(randX, randY));
        randomPoint = randomPoint.x * u + randomPoint.y * v + randomPoint.z * w;
        
        float3 newDir = normalize(randomPoint - worldOrigin);
        newDir = normalize(newDir);
        float cosTheta = max(dot(normal, newDir), 0);
        baseColor *= cosTheta;
        
        payload.cumulativeColor = payload.cumulativeColor *= baseColor;
        payload.currentDepth += 1;
        
        RayDesc ray;
        ray.Origin = worldOrigin;
        ray.Direction = newDir;
        ray.TMin = 0.01;
        ray.TMax = 100000;
        
        TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 2, 0, 2, ray, payload);
    }
    
    else
    {
        float3 lightPos = float3(lightPosition);
    
        float3 worldOrigin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
        float3 lightDir = normalize(lightPos - worldOrigin);
        
        RayDesc ray;
        ray.Origin = worldOrigin;
        ray.Direction = lightDir;
        ray.TMin = 0.01;
        ray.TMax = 100000;
        
        ShadowHitInfo shadowPayload;
        shadowPayload.isHit = false;
        
        TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 1, 0, 1, ray, shadowPayload);
        
        payload.cumulativeColor = shadowPayload.isHit ? payload.cumulativeColor : float3(0.0f, 0.0f, 0.0f);
    }
}

[shader("miss")]
void ScatterMiss(inout ScatterHitInfo payload : SV_RayPayload)
{
    payload.cumulativeColor = float3(0.0f, 0.0f, 0.0f);
}