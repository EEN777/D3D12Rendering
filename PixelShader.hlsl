Texture2D textureSampler : register(t0);
SamplerState samplera : register(s0);

struct PixelShaderInput
{
    float4 Position : SV_Position;
    float2 TextureCoordinate : TEXCOORD;
};

float4 main( PixelShaderInput IN ) : SV_TARGET
{
    float4 texColor = textureSampler.Sample(samplera, IN.TextureCoordinate);
    return texColor;
}