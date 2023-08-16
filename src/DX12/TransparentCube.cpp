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

#include "stdafx.h"
#include "TransparentCube.h"


void TransparentCube::OnCreate(Device * pDevice)
{
	// Read in the file.
	D3D12_SHADER_BYTECODE shaderByteCodeVS = {}, shaderByteCodePS = {};
	CompileShaderFromFile("TransparentCube.hlsl", NULL, "VSMain", "-T vs_6_0", &shaderByteCodeVS);
	CompileShaderFromFile("TransparentCube.hlsl", NULL, "PSMain", "-T ps_6_0", &shaderByteCodePS);

	ThrowIfFailed(pDevice->GetDevice()->CreateRootSignature(0, shaderByteCodePS.pShaderBytecode, shaderByteCodePS.BytecodeLength, IID_PPV_ARGS(&m_pRootSignature)));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
	desc.VS = shaderByteCodeVS;
	desc.PS = shaderByteCodePS;
	desc.pRootSignature = m_pRootSignature;
	desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	desc.DepthStencilState.DepthEnable = FALSE;
	desc.DepthStencilState.StencilEnable = FALSE;
	desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	desc.BlendState.RenderTarget[0] = D3D12_RENDER_TARGET_BLEND_DESC
	{
		TRUE,
		FALSE,
		D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL,
	};
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleMask = UINT_MAX;

	ThrowIfFailed(pDevice->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pPipeline)));
}

void TransparentCube::Draw(ID3D12GraphicsCommandList* pCommandList, const Vectormath::Matrix4& transform, const Vectormath::Vector4& position, const Vectormath::Vector4& radius, const Vectormath::Vector4& color)
{
	pCommandList->IASetIndexBuffer(nullptr);
	pCommandList->IASetVertexBuffers(0, 0, 0);
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->SetPipelineState(m_pPipeline);
	pCommandList->SetGraphicsRootSignature(m_pRootSignature);

	struct transparentCubeConstants
	{
		Vectormath::Matrix4 transform;
		Vectormath::Vector4 position;
		Vectormath::Vector4 radius;
		Vectormath::Vector4 color;
	} constants{transform, position, radius, color};
	pCommandList->SetGraphicsRoot32BitConstants(0, sizeof(constants)/4, &constants, 0);

	pCommandList->DrawInstanced(36, 1, 0, 0);
}

void TransparentCube::OnDestroy()
{
	m_pPipeline->Release();
}

