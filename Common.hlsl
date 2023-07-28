// Hit information, aka ray payload
// This sample only carries a shading color and hit distance.
// Note that the payload should be kept as small as possible,
// and that its size must be declared in the corresponding
// D3D12_RAYTRACING_SHADER_CONFIG pipeline subobjet.

#define PI 3.141592f

struct HitInfo
{
  float4 colorAndDistance;
  uint2 pixelCoords;
};

struct IndirectHitInfo
{
    float3 color;
    uint currentDepth;
    uint2 pixelCoords;
};

struct MaterialProperties
{
    bool isLight;
};

// Attributes output by the raytracing when hitting a surface,
// here the barycentric coordinates
struct Attributes
{
  float2 bary;
};

struct VertexAttributes
{
    float3 position;
    float2 uv;
};

float2 SampleUniformDiskConcentric(float2 hitPoint)
{
    float2 uOffset = 2 * hitPoint - float2(1.0f, 1.0f);
    if (uOffset.x == 0 && uOffset.y == 0)
    {
        return float2(0.0f, 0.0f);
    }
    
    float theta, r;
    if (abs(uOffset.x) > abs(uOffset.y))
    {
        r = uOffset.x;
        theta = ((PI / 4) * (uOffset.y / uOffset.x));
    }
    else
    {
        r = uOffset.y;
        theta = ((PI / 2) - ((PI / 4) * (uOffset.x / uOffset.y)));
    }

    return r * float2(cos(theta), sin(theta));
}

float3 SampleUniformHemisphere(float2 hitPoint)
{
    float z = hitPoint.x;
    float r = sqrt(1 - (z * z));
    float phi = 2 * PI * hitPoint.y;

    return (r * cos(phi), r * sin(phi), z);
}

float3 SampleCosineHemisphere(float2 hitPoint)
{
    float2 d = SampleUniformDiskConcentric(hitPoint);
    float z = sqrt(1 - (d.x * d.x) - (d.y * d.y));
    return float3(d.x, d.y, z);
}

//float

//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#ifndef RANDOMNUMBERGENERATOR_H
#define RANDOMNUMBERGENERATOR_H

// Ref: http://www.reedbeta.com/blog/quick-and-easy-gpu-random-numbers-in-d3d11/
namespace RNG
{
    // Create an initial random number for this thread
    uint SeedThread(uint seed)
    {
        // Thomas Wang hash 
        // Ref: http://www.burtleburtle.net/bob/hash/integer.html
        seed = (seed ^ 61) ^ (seed >> 16);
        seed *= 9;
        seed = seed ^ (seed >> 4);
        seed *= 0x27d4eb2d;
        seed = seed ^ (seed >> 15);
        return seed;
    }

    // Generate a random 32-bit integer
    uint Random(inout uint state)
    {
        // Xorshift algorithm from George Marsaglia's paper.
        state ^= (state << 13);
        state ^= (state >> 17);
        state ^= (state << 5);
        return state;
    }

    // Generate a random float in the range [0.0f, 1.0f)
    float Random01(inout uint state)
    {
        return asfloat(0x3f800000 | Random(state) >> 9) - 1.0;
    }

    // Generate a random float in the range [0.0f, 1.0f]
    float Random01inclusive(inout uint state)
    {
        return Random(state) / float(0xffffffff);
    }


    // Generate a random integer in the range [lower, upper]
    uint Random(inout uint state, uint lower, uint upper)
    {
        return lower + uint(float(upper - lower + 1) * Random01(state));
    }
}
#endif // RANDOMNUMBERGENERATOR_H