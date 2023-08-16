// AMD SampleDX12 sample code
// 
// Copyright(c) 2023 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

uint maxIterations:register(b0);

float3 mandelbrotWithColor(float2 uv)
{
    float fragScaleX = abs((-2.0 - 0.47)) * uv.x + -2.0;
    float fragScaleY = abs((-1.12 - 1.12)) * uv.y + -1.12;

    float x = 0.0;
    float y = 0.0;
    uint iteration = 0;

    while (((x * x) + (y * y)) <= 4.0 && iteration < maxIterations)
    {
        float xtemp = x * x - y * y + fragScaleX;
        y = 2.0 * x * y + fragScaleY;
        x = xtemp;
        iteration = iteration + 1;
    }

    float3 color;
    color.xyz = sqrt(float(iteration) / maxIterations).xxx;

    return color;
}

[RootSignature("RootFlags(0), RootConstants(num32BitConstants = 1, b0)")]
float4 PSMain(in float4 fragCoord : SV_Position) :SV_Target0
{
    // Normalized pixel coordinates (from 0 to 1)
    //float2 uv = (fragCoord.xy + 1.0) * 0.5;
    float2 uv = fragCoord.xy / 1024.0;
    // Time varying pixel color
    float3 col = mandelbrotWithColor(uv);
    // Output to screen
    return float4(col, 1.0);
}

static const float2 triPos[3] =
{
    {-3,1},
    {1, 1},
    {1,-3}
};

[RootSignature("RootFlags(0), RootConstants(num32BitConstants = 1, b0)")]
float4 VSMain(in uint vertexId:SV_VertexID) : SV_Position
{
    return float4(triPos[vertexId].x,triPos[vertexId].y,1,1);
}