struct VSInput {
    float3 Pos;
};

struct ShaderData {
    float4x4 model;
};

struct VSOutput {
    float4 Pos : SV_POSITION;
};

[shader("vertex")]
VSOutput main(VSInput input, uniform ShaderData *shaderData, uint instanceIndex : SV_VulkanInstanceID) {
    VSOutput output;
    float4x4 modelMat = shaderData->model;
    output.Pos = mul(modelMat, float4(input.Pos.xyz, 1.0));
    return output;
}

[shader("fragment")]
float4 main(VSOutput input) {
    return float4(1.0, 1.0, 1.0, 1.0);
}