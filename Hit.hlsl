#include "Common.hlsl"

struct STriVertex
{
    float3 vertex;
    float4 color;
};

StructuredBuffer<STriVertex> BTriVertex : register(t0);
StructuredBuffer<int> indices : register(t1);

[shader("closesthit")] 
void ClosestHit(inout HitInfo payload, Attributes attrib) 
{
    float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

    float3 hitColor = BTriVertex[indices[0]].color * barycentrics.x +
                      BTriVertex[indices[1]].color * barycentrics.y +
                      BTriVertex[indices[2]].color * barycentrics.z;

    payload.colorAndDistance = float4(hitColor, RayTCurrent());
}
