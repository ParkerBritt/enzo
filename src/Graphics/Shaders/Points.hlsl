// Draws each point as a white disc flat to the screen. The whole disc takes the depth of its
// centre, so a surface the point sits on never cuts through it.

struct VertexOutput
{
    float4 clipPosition : SV_Position;
    float2 corner : CORNER;
};

// Diameter of a disc in interface pixels.
static const float discDiameter = 3.5;

// Fraction of the distance to the camera a disc is moved toward it, so a point on a face draws over it.
static const float cameraPull = 0.01;

VertexOutput vertexMain(float2 corner : ATTRIB0, float3 pointPosition : ATTRIB1)
{
    float3 pulledPosition = lerp(pointPosition, cameraPosition.xyz, cameraPull);
    float4 centreClipPosition = mul(viewProjection, float4(pulledPosition, 1.0));

    // Converts the corner from pixels to clip units, which span 2 across the viewport.
    float2 cornerPixels = corner * discDiameter * pixelRatio;
    float2 cornerClipOffset = cornerPixels * 2.0 / viewportPixelSize * centreClipPosition.w;

    VertexOutput output;
    output.clipPosition = centreClipPosition + float4(cornerClipOffset, 0.0, 0.0);
    output.corner = corner;
    return output;
}

float4 pixelMain(VertexOutput input) : SV_Target
{
    if (length(input.corner) > 0.5) discard;
    return float4(1.0, 1.0, 1.0, 1.0);
}
