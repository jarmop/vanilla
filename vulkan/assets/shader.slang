struct VSInput {
    float3 Pos;
};

struct ShaderData {
    float4x4 projection;
    float4x4 view;
    float4x4 model;
    uint32_t selected;
};

struct VSOutput {
    float4 Pos : SV_POSITION;
};

[shader("vertex")]
VSOutput main(VSInput input, uniform ShaderData *shaderData, uint instanceIndex : SV_VulkanInstanceID) {
    VSOutput output;
    float4x4 modelMat = shaderData->model;
    output.Pos = mul(shaderData->projection, mul(shaderData->view, mul(modelMat, float4(input.Pos.xyz, 1.0))));
    float4 fragPos = mul(mul(shaderData->view, modelMat), float4(input.Pos.xyz, 1.0));
    return output;
}

[shader("fragment")]
float4 main(VSOutput input) {
    return float4(1.0, 1.0, 1.0, 1.0);
}