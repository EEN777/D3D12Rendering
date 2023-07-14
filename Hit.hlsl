#include "Common.hlsl"

struct STriVertex
{
    float3 vertex;
    float3 TexCoord;
};


ByteAddressBuffer vertices : register(t0);
ByteAddressBuffer modelIndices : register(t1);
Texture2D<float4> texture[] : register(t2);
StructuredBuffer<STriVertex> verticesTest[] : register(t3);

uint3 GetIndices(uint triangleIndex)
{
    uint baseIndex = (triangleIndex * 3);
    int address = (baseIndex * 4);
    return modelIndices.Load3(address);
}

VertexAttributes GetVertexAttributes(uint triangleIndex, float3 barycentrics)
{
    uint3 indices = GetIndices(triangleIndex);
    VertexAttributes v;
    v.position = float3(0, 0, 0);
    v.uv = float2(0, 0);

    for (uint i = 0; i < 3; i++)
    {
        STriVertex vertexData = verticesTest[0][indices[i]];
        v.position += vertexData.vertex * barycentrics[i];
        v.uv += vertexData.TexCoord * barycentrics[i];
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
    float3 color = texture[InstanceID()].Load(int3(coord, 0)).rgb;
    
    payload.colorAndDistance = float4(color.rgb, RayTCurrent());
}
