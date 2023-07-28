#include "Common.hlsl"

// Raytracing output texture, accessed as a UAV
RWTexture2D< float4 > gOutput : register(u0);
RWTexture2D< float4 > gAccumulated : register(u1);

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0);

cbuffer CameraParams : register(b0)
{
    float4x4 view;
    float4x4 projection;
    float4x4 viewI;
    float4x4 projectionI;
}

cbuffer FrameInfo : register(b1)
{
    uint frameNumber;
    uint frameAccumulationStarted;
    bool isAccumulating;
    bool isSuperSampling;  
};

[shader("raygeneration")] 
void RayGen() 
{    
    HitInfo payload;
    payload.colorAndDistance = float4(0.9, 0.6, 0.2, 1);
    
    bool antiAliasing = true;
     
    uint2 launchIndex = DispatchRaysIndex().xy;
    float2 dims = float2(DispatchRaysDimensions().xy);
    
    payload.pixelCoords = launchIndex;
    
    if (!isAccumulating)
    {
        gAccumulated[launchIndex] = float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    if (isSuperSampling)
    {
        float3 cumulativeColor = float3(0.0f, 0.0f, 0.0f);
        float2 offsets[4] = { float2(-0.25, -0.25), float2(0.25, -0.25), float2(-0.25, 0.25), float2(0.25, 0.25) };
        for (int i = 0; i < 4; ++i)
        {
            float2 d = (((launchIndex.xy + 0.5f + offsets[i]) / dims.xy) * 2.f - 1.f);
            RayDesc ray;
    
            ray.Origin = mul(viewI, float4(0, 0, 0, 1));
            float4 target = mul(projectionI, float4(d.x, -d.y, 1, 1));
            ray.Direction = mul(viewI, float4(target.xyz, 0));
            ray.TMin = 0;
            ray.TMax = 100000;
    
            TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
            cumulativeColor += payload.colorAndDistance.rgb;
        }
   
        cumulativeColor /= 4;
        
        if (isAccumulating)
        {
            gAccumulated[launchIndex] = gAccumulated[launchIndex] + float4(cumulativeColor, 1.f);
            gOutput[launchIndex] = gAccumulated[launchIndex] / ((frameNumber - frameAccumulationStarted) + 1);
        }
        
        else
        {
            gOutput[launchIndex] = float4(cumulativeColor, 1.f);
        }
    }
    else
    {
        float2 d = (((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f);
        RayDesc ray;
    
        ray.Origin = mul(viewI, float4(0, 0, 0, 1));
        float4 target = mul(projectionI, float4(d.x, -d.y, 1, 1));
        ray.Direction = mul(viewI, float4(target.xyz, 0));
        ray.TMin = 0;
        ray.TMax = 100000;
    
        TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
        if (isAccumulating)
        {
            gAccumulated[launchIndex] = gAccumulated[launchIndex] + float4(payload.colorAndDistance.rgb, 1.f);
            gOutput[launchIndex] = gAccumulated[launchIndex] / ((frameNumber - frameAccumulationStarted) + 1);
        }
        else
        {
            gOutput[launchIndex] = float4(payload.colorAndDistance.rgb, 1.f);
        }

    }
}
