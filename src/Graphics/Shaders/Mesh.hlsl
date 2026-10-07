// Draws the display mesh lit by a fixed light, and its wireframe in flat grey.

struct VertexOutput
{
    float4 clipPosition : SV_Position;
    float3 normal : NORMAL;
};

static const float3 lightDirection = normalize(float3(1.0, 1.0, 1.0));
static const float3 wireframeColor = float3(0.4, 0.4, 0.4);

VertexOutput vertexMain(float3 position : ATTRIB0, float3 normal : ATTRIB1)
{
    VertexOutput output;
    output.clipPosition = mul(viewProjection, float4(position, 1.0));
    output.normal = normal;
    return output;
}

// Scales the light from 0.5 on faces turned away from it to 1 on faces facing it.
float4 shadedPixelMain(VertexOutput input) : SV_Target
{
    float brightness = lerp(0.5, 1.0, dot(input.normal, lightDirection) * 0.5 + 0.5);
    return float4(geometryColor.rgb * brightness, 1.0);
}

float4 wireframePixelMain(VertexOutput input) : SV_Target
{
    return float4(wireframeColor, 1.0);
}
