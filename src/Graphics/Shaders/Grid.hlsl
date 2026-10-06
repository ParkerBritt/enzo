// Draws the ground grid, fading out with distance from the origin.

cbuffer Constants
{
    float4x4 viewProjection;
};

struct VertexOutput
{
    float4 clipPosition : SV_Position;
    float3 worldPosition : WORLD_POSITION;
};

static const float3 lineColor = float3(0.227, 0.227, 0.275);
static const float centerOpacity = 0.5;
static const float fadeDistance = 40.0;

VertexOutput vertexMain(float3 position : ATTRIB0)
{
    VertexOutput output;
    output.clipPosition = mul(viewProjection, float4(position, 1.0));
    output.worldPosition = position;
    return output;
}

float4 pixelMain(VertexOutput input) : SV_Target
{
    float fade = saturate(1.0 - length(input.worldPosition) / fadeDistance);
    return float4(lineColor, centerOpacity * fade);
}
