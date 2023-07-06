#include "Common.hlsl"

struct STriVertex
{
    float3 vertex;
    float3 TexCoord;
};

StructuredBuffer<STriVertex> BTriVertex : register(t0);
StructuredBuffer<uint> indices : register(t1);
//Texture2D<float4> texture : register(t2);

[shader("closesthit")] 
void ClosestHit(inout HitInfo payload, Attributes attrib) 
{
    float3 barycentrics = float3(1.f - attrib.bary.x - attrib.bary.y, attrib.bary.x, attrib.bary.y);

	uint startIndex = 3 * PrimitiveIndex();

	float3 hitColor = BTriVertex[indices[startIndex]].TexCoord * barycentrics.x +
		BTriVertex[indices[startIndex + 1]].TexCoord * barycentrics.y +
		BTriVertex[indices[startIndex + 2]].TexCoord * barycentrics.z;
    
    //float3 color = texture.Load(int3(0, 0, 0).rgb);

    payload.colorAndDistance = float4(hitColor, RayTCurrent());
}
