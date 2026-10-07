// Draws each solo point as a white disc facing the camera.

struct VertexOutput
{
    float4 clipPosition : SV_Position;
    float2 corner : CORNER;
};

// World size of a disc per unit of distance from the camera, which keeps discs one size on screen.
static const float sizePerDistance = 0.005;

VertexOutput vertexMain(float2 corner : ATTRIB0, float3 pointPosition : ATTRIB1)
{
    float3 cameraRight = view[0].xyz;
    float3 cameraUp = view[1].xyz;
    float size = distance(pointPosition, cameraPosition.xyz) * sizePerDistance;
    float3 worldPosition = pointPosition + (cameraRight * corner.x + cameraUp * corner.y) * size;

    VertexOutput output;
    output.clipPosition = mul(viewProjection, float4(worldPosition, 1.0));
    output.corner = corner;
    return output;
}

float4 pixelMain(VertexOutput input) : SV_Target
{
    if (length(input.corner) > 0.5) discard;
    return float4(1.0, 1.0, 1.0, 1.0);
}
