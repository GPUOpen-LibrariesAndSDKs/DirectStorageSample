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

#pragma once

class ArtificialWorkload
{
	ID3D12PipelineState* m_pPipeline = nullptr;
	ID3D12RootSignature* m_pRootSignature = nullptr;
	ID3D12Resource* m_pRenderTargetResource = nullptr;
	ID3D12DescriptorHeap* m_pRTVDescriptorHeap = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE m_RTVDescriptorHandle = {};

	public:
	void OnCreate(Device* pDevice);
	void Draw(ID3D12GraphicsCommandList* pCommandList, uint32_t iterations);
	void OnDestroy();
};