// Declares the frame constants every pass reads.

cbuffer FrameConstants
{
    float4x4 viewProjection;
    float4 cameraPosition;
    float4 geometryColor;
    float2 viewportPixelSize;
    float pixelRatio;
    float padding;
};
