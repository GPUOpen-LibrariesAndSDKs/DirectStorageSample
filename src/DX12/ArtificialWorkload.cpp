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
#include "ArtificialWorkload.h"


void ArtificialWorkload::OnCreate(Device * pDevice)
{
	// Read in the file.
	std::string Workload = "";
	D3D12_SHADER_BYTECODE shaderByteCodeVS = {}, shaderByteCodePS = {};
	CompileShaderFromFile("Mandelbrot.hlsl", NULL, "VSMain", "-T vs_6_0", &shaderByteCodeVS);
	CompileShaderFromFile("Mandelbrot.hlsl", NULL, "PSMain", "-T ps_6_0", &shaderByteCodePS);

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
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	desc.NumRenderTargets = 1;
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleMask = UINT_MAX;

	ThrowIfFailed(pDevice->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pPipeline)));

	// Go ahead and create a rendertarget just for this.
	auto rtDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1024, 1024, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(pDevice->GetDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &rtDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(&m_pRenderTargetResource)));

	// And our own RTV descriptor heap.
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescHeapDesc = {};
	rtvDescHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvDescHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescHeapDesc.NumDescriptors = 1;
	ThrowIfFailed(pDevice->GetDevice()->CreateDescriptorHeap(&rtvDescHeapDesc, IID_PPV_ARGS(&m_pRTVDescriptorHeap)));

	m_RTVDescriptorHandle = m_pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = rtDesc.Format;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	pDevice->GetDevice()->CreateRenderTargetView(m_pRenderTargetResource, &rtvDesc, m_RTVDescriptorHandle);
}

void ArtificialWorkload::Draw(ID3D12GraphicsCommandList* pCommandList, uint32_t iterations)
{
	if (iterations > 0)
	{
		//pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargetResource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET));

		pCommandList->IASetIndexBuffer(nullptr);
		pCommandList->IASetVertexBuffers(0, 0, 0);
		pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pCommandList->SetPipelineState(m_pPipeline);
		pCommandList->SetGraphicsRootSignature(m_pRootSignature);
		pCommandList->SetGraphicsRoot32BitConstant(0, iterations, 0);
		pCommandList->OMSetRenderTargets(1, &m_RTVDescriptorHandle, FALSE, nullptr);

		// Set the viewport
		D3D12_VIEWPORT viewPort = { 0, 0, 1024, 1024, -1.0f, 1.0f };
		pCommandList->RSSetViewports(1, &viewPort);

		// Create scissor rectangle
		D3D12_RECT rectScissor = { 0, 0, 1024, 1024 };
		pCommandList->RSSetScissorRects(1, &rectScissor);

		pCommandList->DrawInstanced(3, 1, 0, 0);

		//pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargetResource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON));
	}
}

void ArtificialWorkload::OnDestroy()
{
	m_pPipeline->Release();
	m_pRenderTargetResource->Release();
	m_pRTVDescriptorHeap->Release();
	m_pRootSignature->Release();
	m_RTVDescriptorHandle = {};
}

