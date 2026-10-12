// Draws each point light as a ring with rays, flat to the screen and tinted with the light's color.

struct VertexOutput
{
    float4 clipPosition : SV_Position;
    float3 color : COLOR;
};

VertexOutput vertexMain(float2 offsetPixels : ATTRIB0, float3 lightPosition : ATTRIB1, float3 lightColor : ATTRIB2)
{
    float4 centreClipPosition = mul(viewProjection, float4(lightPosition, 1.0));

    // Converts the offset from pixels to clip units, which span 2 across the viewport.
    float2 offsetClip = offsetPixels * pixelRatio * 2.0 / viewportPixelSize * centreClipPosition.w;

    VertexOutput output;
    output.clipPosition = centreClipPosition + float4(offsetClip, 0.0, 0.0);
    output.color = lightColor;
    return output;
}

float4 pixelMain(VertexOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}
