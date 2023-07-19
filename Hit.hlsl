#include "Common.hlsl"

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

struct ShadowHitInfo
{
    bool isHit;
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
void ClosestHit(inout HitInfo payload, Attributes attrib)
{
    if (InstanceID() < 7)
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
    
        
        // Find the world - space hit position
        float3 worldOrigin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();

        //normals exported from Unreal, inverting them here.
        float3 lightDir = normalize(lightPos - worldOrigin);
        normal = normalize(normal);
        normal = normal * -1;
        
        //This would need to be adjusted if the models were moved.
        //float3 worldNormal = mul(normal, float3x3(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f));
        //worldNormal = normalize(worldNormal);
        
        float ambientLightIntensity = 0.01f;
        
        float NdotL = max(dot(normal, lightDir), 0.0f);
        
        //NdotL = NdotL > 0 ? NdotL : -1.0f * NdotL;
        
        float dist = length(worldOrigin - lightPos);
        
        float radius = 2000;
        
        RayDesc ray;
        ray.Origin = worldOrigin;
        ray.Direction = lightDir;
        ray.TMin = 0.01;
        ray.TMax = 100000; // Slight optimization possible? Was originally a TMax of 100000 but now using the calulated distance from the light source.
        bool hit = true;

        // Initialize the ray payload
        ShadowHitInfo shadowPayload;
        shadowPayload.isHit = false;
        
      // Trace the ray
      TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 1, 0, 1, ray, shadowPayload);
        float factor = shadowPayload.isHit ? 0.3 : 1.0;
        //uint sampleCount = 32;
        
        //float3 u, v, w;
        //w = normal;
        //u = normalize(cross((abs(w.x) > 0.1 ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f)), w));
        //v = cross(w, u);
        
        ////[unroll]
        //for (uint i = 0; i < sampleCount; ++i)
        //{
        //    uint state = RNG::SeedThread(777 * i);
        //    float randX = RNG::Random01(state);
        //    state = RNG::SeedThread(322 * i);
        //    float randY = RNG::Random01(state);
        //    float3 randomPoint = SampleCosineHemisphere(float2(randX, randY));
        //    randomPoint = randomPoint.x * u + randomPoint.y * v + randomPoint.z * w;
        //    randomPoint = worldOrigin + randomPoint;
        
        //    lightDir = normalize(lightPos - randomPoint);
        //    ray.Origin = randomPoint;
        //    ray.Direction = lightDir;
        //    // Trace the ray
        //    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 1, 0, 1, ray, shadowPayload);
        //    factor += shadowPayload.isHit ? 0.3 : 1.0;
        //}
        
        //factor = factor / (sampleCount + 1);
        
        float3 lightColor = float3(1.0f, 1.0f, 1.0f);
        //float lightIntensity = 100;
        //lightColor *= lightIntensity;
        
        float3 baseColor = color.rgb;
        
        if (dist > radius)
        {
            baseColor = baseColor * ambientLightIntensity;
        }
        
        else
        {
            float ratio = max((1.0f - (dist / radius)), 0.0f);
            baseColor = baseColor * (ratio * lightColor * NdotL + ambientLightIntensity);
        }
        
        ScatterHitInfo scatterPayload;
        scatterPayload.cumulativeColor = baseColor;
        scatterPayload.currentDepth = 1;
        scatterPayload.maxDepth = 4;
        
        float3 u, v, w;
        w = normal;
        u = normalize(cross((abs(w.x) > 0.1 ? float3(0.0f, 1.0f, 0.0f) : float3(1.0f, 0.0f, 0.0f)), w));
        v = cross(w, u);
        
        uint sampleCount = 10000;
        
        float3 sampledColor = baseColor; //float3(0.0f, 0.0f, 0.0f);
        for (uint i = 0; i < sampleCount; ++i)
        {
            uint state = RNG::SeedThread(777 * i);
            float randX = RNG::Random01(state);
            state = RNG::SeedThread(322 * i);
            float randY = RNG::Random01(state);
            float3 randomPoint = SampleCosineHemisphere(float2(randX, randY));
            randomPoint = randomPoint.x * u + randomPoint.y * v + randomPoint.z * w;
        
            float3 newDir = normalize(randomPoint - worldOrigin);
            newDir = normalize(newDir);
            float cosTheta = max(dot(normal, newDir), 0);
            baseColor *= cosTheta;
        
            RayDesc scatterRay;
            scatterRay.Origin = worldOrigin;
            scatterRay.Direction = newDir;
            scatterRay.TMin = 0.01;
            scatterRay.TMax = 100000;
        
            TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 2, 0, 2, scatterRay, scatterPayload);
            //scatterPayload.cumulativeColor /= scatterPayload.currentDepth;
            //sampledColor += baseColor * cosTheta * scatterPayload.cumulativeColor /** (4 * PI)*/;
            sampledColor += scatterPayload.cumulativeColor;
        }
        
        //sampledColor = sampledColor / (sampleCount + 1);
        float3 finalColor = sampledColor * factor;
        
        //finalColor = finalColor * 257;

        float4 hitColor = float4(finalColor, RayTCurrent());
        payload.colorAndDistance = float4(hitColor);
    }
}
