#include "Common.hlsl"

struct STriVertex
{
    float3 vertex;
    float3 TexCoord;
};


ByteAddressBuffer vertices : register(t0);
ByteAddressBuffer indices : register(t1);
Texture2D<float4> texture : register(t2);

uint3 GetIndices(uint triangleIndex)
{
    uint baseIndex = (triangleIndex * 3);
    int address = (baseIndex * 4);
    return indices.Load3(address);
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
        v.position += asfloat(vertices.Load3(address)) * barycentrics[i];
        address += (3 * 4);
        v.uv += asfloat(vertices.Load2(address)) * barycentrics[i];
    }

    return v;
}

[shader("closesthit")] 
void ClosestHit(inout HitInfo payload, Attributes attrib) 
{   
    uint triangleIndex = PrimitiveIndex();
    float3 barycentrics = float3((1.0f - attrib.bary.x - attrib.bary.y), attrib.bary.x, attrib.bary.y);
    VertexAttributes vertex = GetVertexAttributes(triangleIndex, barycentrics);

    int2 coord = int2(vertex.uv.x * 2048, vertex.uv.y * 1024);
    float3 color = texture.Load(int3(coord, 0)).rgb;
    
    payload.colorAndDistance = float4(color.rgb, RayTCurrent());
}
