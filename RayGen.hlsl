#include "Common.hlsl"

// Raytracing output texture, accessed as a UAV
RWTexture2D< float4 > gOutput : register(u0);

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0);

cbuffer CameraParams : register(b0)
{
    float4x4 view;
    float4x4 projection;
    float4x4 viewI;
    float4x4 projectionI;
}

[shader("raygeneration")] 
void RayGen() {
    
  HitInfo payload;
  payload.colorAndDistance = float4(0.9, 0.6, 0.2, 1);

  uint2 launchIndex = DispatchRaysIndex().xy;
  float2 dims = float2(DispatchRaysDimensions().xy);
  float2 d = (((launchIndex.xy + 0.5f) / dims.xy) * 2.f - 1.f);
    
    RayDesc ray;
    //ray.Origin = float3(d.x, -d.y, 1.0f);
    //ray.Direction = float3(a, 0, -1);
    //ray.TMin = 0;
    //ray.TMax = 100000;
    
    ray.Origin = mul(viewI, float4(0, 0, 0, 1));
    float4 target = mul(projectionI, float4(d.x, -d.y, 1, 1));
    ray.Direction = mul(viewI, float4(target.xyz, 0));
    ray.TMin = 0;
    ray.TMax = 100000;
    
    TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
   

  gOutput[launchIndex] = float4(payload.colorAndDistance.rgb, 1.f);
}
