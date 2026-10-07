// Declares the frame constants every pass reads.

cbuffer FrameConstants
{
    float4x4 viewProjection;
    float4x4 view;
    float4 cameraPosition;
    float4 geometryColor;
};
