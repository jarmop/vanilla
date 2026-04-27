struct VertexInput {
    float2 pos;
    float3 color;
};

struct VertexOutput {
    float3 color;
    float4 pos : SV_Position;
};

[shader("vertex")]
VertexOutput vertMain(VertexInput in) {
    VertexOutput out;
    out.pos = float4(in.pos, 0.0, 1.0);
    out.color = in.color;
    return out;
}

[shader("fragment")]
float4 fragMain(VertexOutput in) : SV_Target {
    return float4(in.color, 1.0);
}
