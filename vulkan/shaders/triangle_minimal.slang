static float2 positions[3] = float2[](
    float2(0.0, 0.0),
    float2(0.5, 0.0),
    float2(0.5, 0.5)
);

[shader("vertex")]
float4 vertMain(uint i : SV_VertexID) : SV_Position {
    // The last value of the vector is used to divide the rest of the vector values when
    // normalizing. Setting it to "1.0" makes it not have any effect.
    return float4(positions[i], 0.0, 1.0);
}

[shader("fragment")]
float4 fragMain() : SV_Target
{
    return float4(1.0, 0.6, 0.2, 1.0);
}