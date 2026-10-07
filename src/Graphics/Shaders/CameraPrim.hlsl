// Draws each camera as an orange frame placed by its transform.

static const float3 frameColor = float3(0.9, 0.7, 0.2);

float4 vertexMain(
    float3 position : ATTRIB0,
    float4 transformColumn0 : ATTRIB1,
    float4 transformColumn1 : ATTRIB2,
    float4 transformColumn2 : ATTRIB3,
    float4 transformColumn3 : ATTRIB4
) : SV_Position
{
    float4 worldPosition = transformColumn0 * position.x + transformColumn1 * position.y +
                           transformColumn2 * position.z + transformColumn3;
    return mul(viewProjection, worldPosition);
}

float4 pixelMain() : SV_Target
{
    return float4(frameColor, 1.0);
}
