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

struct CubeConstants
{
    float4x4 transform;
    float4 position;
    float4 radius;
    float4 color;
};
CubeConstants cubeConstants:register(b0);
[RootSignature("RootFlags(0), RootConstants(num32BitConstants = 28, b0)")]

// PS Main
float4 PSMain() : SV_Target0
{
    return cubeConstants.color;
}

static const float3 vertPos[8] =
{
    // -z side.
    {-1, -1, -1}, // 0
    {-1,  1, -1}, // 1
    { 1,  1, -1}, // 2
    { 1, -1, -1}, // 3
    
    // +z side
    {-1, -1,  1}, // 4
    {-1,  1,  1}, // 5
    { 1,  1,  1}, // 6
    { 1, -1,  1}  // 7

};

static const uint indexBuffer[36] =
{
    0,1,2,0,2,3, // -z side.
    4,5,6,4,6,7, // +z side.
    0,4,7,0,7,3, // -x side.
    1,5,6,1,6,2, // +x side.
    0,4,5,0,5,1, // -y side.
    3,7,6,3,2,6  // +y side
};


// VSMain
float4 VSMain(in uint vertexId:SV_VertexID) : SV_Position
{
    float4 currentVert = float4(vertPos[indexBuffer[vertexId]],1.0);
    return mul(cubeConstants.transform, float4(cubeConstants.position + currentVert * cubeConstants.radius.xyz, 1.0f));
}