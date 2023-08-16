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

#include "Renderer.h"
#include "UI.h"
#include "GLTFTextureAndBuffersDirectStorage.h"
#include "Misc/CPUUserMarkers.h"
#include "SampleOptions.h"
#include <stdlib.h>


//--------------------------------------------------------------------------------------
//
// OnCreate
//
//--------------------------------------------------------------------------------------
void Renderer::OnCreate(Device* pDevice, SwapChain* pSwapChain, AsyncPool* pAsyncPool, float FontSize, const std::unordered_map<std::string, ScenePathPair>* scenePathMap, std::vector<StreamingVolume>* streamingVolumes, const SampleOptions* const sampleOptions)
{
    m_pDevice = pDevice;
    m_pSampleOptions = sampleOptions;
    m_pAsyncPool = pAsyncPool;

    // Initialize helpers

    // Create all the heaps for the resources views
    const uint32_t cbvDescriptorCount = 4000;
    const uint32_t srvDescriptorCount = 8000;
    const uint32_t uavDescriptorCount = 10;
    const uint32_t dsvDescriptorCount = 10;
    const uint32_t rtvDescriptorCount = 60;
    const uint32_t samplerDescriptorCount = 20;
    m_ResourceViewHeaps.OnCreate(pDevice, cbvDescriptorCount, srvDescriptorCount, uavDescriptorCount, dsvDescriptorCount, rtvDescriptorCount, samplerDescriptorCount);

    // Create a commandlist ring for the Direct queue
    uint32_t commandListsPerBackBuffer = 8;
    m_CommandListRing.OnCreate(pDevice, backBufferCount, commandListsPerBackBuffer, pDevice->GetGraphicsQueue()->GetDesc());

    // Create a 'dynamic' constant buffer
    const uint32_t constantBuffersMemSize = 2 * 1024 * 1024;
    m_ConstantBufferRing.OnCreate(pDevice, backBufferCount, constantBuffersMemSize);

    // Create a 'static' pool for vertices, indices and constant buffers
    const uint32_t staticGeometryMemSize = 512;
    m_VidMemBufferPool.OnCreate(pDevice, staticGeometryMemSize, true, "StaticGeom");

    // initialize the GPU time stamps module
    m_GPUTimer.OnCreate(pDevice, backBufferCount);

    // Quick helper to upload resources, it has it's own commandList and uses suballocation.
    const uint32_t uploadHeapMemSize = 128 * 1024 * 1024;
    m_UploadHeap.OnCreate(pDevice, uploadHeapMemSize);    // initialize an upload heap (uses suballocation for faster results)

    // Create GBuffer and render passes
    //
    {
        m_GBuffer.OnCreate(
            pDevice,
            &m_ResourceViewHeaps,
            {
                { GBUFFER_DEPTH, DXGI_FORMAT_D32_FLOAT},
                { GBUFFER_FORWARD, DXGI_FORMAT_R16G16B16A16_FLOAT},
                { GBUFFER_MOTION_VECTORS, DXGI_FORMAT_R16G16_FLOAT},
            },
            1
        );

        GBufferFlags fullGBuffer = GBUFFER_DEPTH | GBUFFER_FORWARD | GBUFFER_MOTION_VECTORS;
        m_RenderPassFullGBuffer.OnCreate(&m_GBuffer, fullGBuffer);
        m_RenderPassJustDepthAndHdr.OnCreate(&m_GBuffer, GBUFFER_DEPTH | GBUFFER_FORWARD);
    }

    // Load BRDF Texture.
    m_BRDFLUT.InitFromFile(pDevice, &m_UploadHeap, nullptr, "BrdfLut.dds", 0, false); // LUT images are stored as linear

    m_SkyDome.OnCreate(pDevice, &m_UploadHeap, &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool, "..\\media\\cauldron-media\\envmaps\\papermill\\diffuse.dds", "..\\media\\cauldron-media\\envmaps\\papermill\\specular.dds", DXGI_FORMAT_R16G16B16A16_FLOAT, 4);
    m_SkyDomeProc.OnCreate(pDevice, &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool, DXGI_FORMAT_R16G16B16A16_FLOAT, 1);
    m_Wireframe.OnCreate(pDevice, &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool, DXGI_FORMAT_R16G16B16A16_FLOAT, 1);
    m_WireframeBox.OnCreate(pDevice, &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool);
    m_DownSample.OnCreate(pDevice, &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool, DXGI_FORMAT_R16G16B16A16_FLOAT);
    m_Bloom.OnCreate(pDevice, &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool, DXGI_FORMAT_R16G16B16A16_FLOAT);
    m_TAA.OnCreate(pDevice, &m_ResourceViewHeaps, &m_VidMemBufferPool);
    m_ArtificialWorkload.OnCreate(pDevice);
    m_TransparentCube.OnCreate(pDevice);

    // Create tonemapping pass
    m_ToneMappingPS.OnCreate(pDevice, &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool, pSwapChain->GetFormat());
    m_ToneMappingCS.OnCreate(pDevice, &m_ResourceViewHeaps, &m_ConstantBufferRing);
    m_ColorConversionPS.OnCreate(pDevice, &m_ResourceViewHeaps, &m_ConstantBufferRing, &m_VidMemBufferPool, pSwapChain->GetFormat());

    // Initialize UI rendering resources
    m_ImGUI.OnCreate(pDevice, &m_UploadHeap, &m_ResourceViewHeaps, &m_ConstantBufferRing, pSwapChain->GetFormat(), FontSize);

    // Make sure upload heap has finished uploading before continuing
    m_VidMemBufferPool.UploadData(m_UploadHeap.GetCommandList());
    m_UploadHeap.FlushAndFinish();

    m_pScenePathMap = scenePathMap;
    m_pStreamingVolumes = streamingVolumes;
}

//--------------------------------------------------------------------------------------
//
// OnDestroy
//
//--------------------------------------------------------------------------------------
void Renderer::OnDestroy()
{
    if( m_pAsyncPool) 
        m_pAsyncPool->Flush();


    m_ImGUI.OnDestroy();
    m_ColorConversionPS.OnDestroy();
    m_ToneMappingCS.OnDestroy();
    m_ToneMappingPS.OnDestroy();
    m_TAA.OnDestroy();
    m_Bloom.OnDestroy();
    m_DownSample.OnDestroy();
    m_WireframeBox.OnDestroy();
    m_Wireframe.OnDestroy();
    m_SkyDomeProc.OnDestroy();
    m_SkyDome.OnDestroy();
    m_BRDFLUT.OnDestroy();
    m_GBuffer.OnDestroy();
    m_ArtificialWorkload.OnDestroy();
    m_TransparentCube.OnDestroy();

    m_UploadHeap.OnDestroy();
    m_GPUTimer.OnDestroy();
    m_VidMemBufferPool.OnDestroy();
    m_ConstantBufferRing.OnDestroy();
    m_ResourceViewHeaps.OnDestroy();
    m_CommandListRing.OnDestroy();
}

//--------------------------------------------------------------------------------------
//
// OnCreateWindowSizeDependentResources
//
//--------------------------------------------------------------------------------------
void Renderer::OnCreateWindowSizeDependentResources(SwapChain *pSwapChain, uint32_t Width, uint32_t Height)
{
    m_Width = Width;
    m_Height = Height;

    // Set the viewport & scissors rect
    m_Viewport = { 0.0f, 0.0f, static_cast<float>(Width), static_cast<float>(Height), 0.0f, 1.0f };
    m_RectScissor = { 0, 0, (LONG)Width, (LONG)Height };

    // Create GBuffer
    //
    m_GBuffer.OnCreateWindowSizeDependentResources(pSwapChain, Width, Height);
    m_RenderPassFullGBuffer.OnCreateWindowSizeDependentResources(Width, Height);
    m_RenderPassJustDepthAndHdr.OnCreateWindowSizeDependentResources(Width, Height);
    
    m_TAA.OnCreateWindowSizeDependentResources(Width, Height, &m_GBuffer);

    // update bloom and downscaling effect
    //
    m_DownSample.OnCreateWindowSizeDependentResources(m_Width, m_Height, &m_GBuffer.m_HDR, 5); //downsample the HDR texture 5 times
    m_Bloom.OnCreateWindowSizeDependentResources(m_Width / 2, m_Height / 2, m_DownSample.GetTexture(), 5, &m_GBuffer.m_HDR);
}

//--------------------------------------------------------------------------------------
//
// OnDestroyWindowSizeDependentResources
//
//--------------------------------------------------------------------------------------
void Renderer::OnDestroyWindowSizeDependentResources()
{
    m_Bloom.OnDestroyWindowSizeDependentResources();
    m_DownSample.OnDestroyWindowSizeDependentResources();

    m_GBuffer.OnDestroyWindowSizeDependentResources();

    m_TAA.OnDestroyWindowSizeDependentResources();
}

void Renderer::OnUpdateDisplayDependentResources(SwapChain* pSwapChain)
{
    // Update pipelines in case the format of the RTs changed (this happens when going HDR)
    m_ColorConversionPS.UpdatePipelines(pSwapChain->GetFormat(), pSwapChain->GetDisplayMode());
    m_ToneMappingPS.UpdatePipelines(pSwapChain->GetFormat());
    m_ImGUI.UpdatePipeline((pSwapChain->GetDisplayMode() == DISPLAYMODE_SDR) ? pSwapChain->GetFormat() : m_GBuffer.m_HDR.GetFormat());
}

const std::vector<SceneTimingData const *> Renderer::GetSceneLoadingTimings() const
{
    std::vector<SceneTimingData const *> sceneTimings(m_SceneData.size());

    for (size_t i = 0; i < sceneTimings.size(); i++)
    {
        sceneTimings[i] = &m_SceneData[i]->m_timingData;
    }

    return sceneTimings;
}

//--------------------------------------------------------------------------------------
//
// OnRender
//
//--------------------------------------------------------------------------------------
void Renderer::OnRender(const UIState* pState, const Camera& Cam, const TimeState& timeState, SwapChain* pSwapChain)
{
    m_rollingFrameTimeSnapshot.AddTime(GetLastFrameTiming());

    // Timing values
    UINT64 gpuTicksPerSecond;
    m_pDevice->GetGraphicsQueue()->GetTimestampFrequency(&gpuTicksPerSecond);

    // Let our resource managers do some house keeping
    m_CommandListRing.OnBeginFrame();
    m_ConstantBufferRing.OnBeginFrame();
    m_GPUTimer.OnBeginFrame(gpuTicksPerSecond, &m_TimeStamps);

    std::vector<per_frame*> pPerFrame;
    pPerFrame.reserve(m_SceneData.size());

    for (auto& sceneData : m_SceneData)
    {
        sceneData->m_gltfCommon.SetAnimationTime(0, timeState.m_animationTime);
        sceneData->m_gltfCommon.TransformScene(0, sceneData->m_sceneTransform);

        sceneData->m_constantBuffer.OnBeginFrame();

        auto currentScenePerFrameData = sceneData->m_pTexturesAndBuffers->m_pGLTFCommon->SetPerFrameData(Cam);
        pPerFrame.push_back(currentScenePerFrameData);

        // Set some lighting factors
        currentScenePerFrameData->iblFactor = pState->IBLFactor;
        currentScenePerFrameData->emmisiveFactor = pState->EmissiveFactor;
        currentScenePerFrameData->invScreenResolution[0] = 1.0f / ((float)m_Width);
        currentScenePerFrameData->invScreenResolution[1] = 1.0f / ((float)m_Height);

        currentScenePerFrameData->wireframeOptions.setX(pState->WireframeColor[0]);
        currentScenePerFrameData->wireframeOptions.setY(pState->WireframeColor[1]);
        currentScenePerFrameData->wireframeOptions.setZ(pState->WireframeColor[2]);
        currentScenePerFrameData->wireframeOptions.setW(pState->WireframeMode == UIState::WireframeMode::WIREFRAME_MODE_SOLID_COLOR ? 1.0f : 0.0f);
        currentScenePerFrameData->lodBias = 0.0f;
        sceneData->m_pTexturesAndBuffers->SetPerFrameConstants();
        sceneData->m_pTexturesAndBuffers->SetSkinningMatricesForSkeletons();
    }


    // command buffer calls
    ID3D12GraphicsCommandList* pCmdLst1 = m_CommandListRing.GetNewCommandList();

    m_GPUTimer.GetTimeStamp(pCmdLst1, "Begin Frame");

    pCmdLst1->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pSwapChain->GetCurrentBackBufferResource(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    // Render Scene to the GBuffer ------------------------------------------------
    if (pPerFrame.size() > 0)
    {
        pCmdLst1->RSSetViewports(1, &m_Viewport);
        pCmdLst1->RSSetScissorRects(1, &m_RectScissor);


        {
            const bool bWireframe = pState->WireframeMode != UIState::WireframeMode::WIREFRAME_MODE_OFF;

            // Render opaque geometry
            m_RenderPassFullGBuffer.BeginPass(pCmdLst1, true);
            std::vector<GltfPbrPass::BatchList> opaque, transparent;
            opaque.reserve(8000);
            for (const auto& sceneData : m_SceneData)
            {
                
                // prepare lists with appropriate reservations.


                sceneData->m_pbrPass.BuildBatchLists(&opaque, &transparent, bWireframe);
                sceneData->m_pbrPass.DrawBatchList(pCmdLst1, nullptr, &opaque, bWireframe);
                opaque.clear();
            }

            m_GPUTimer.GetTimeStamp(pCmdLst1, "PBR Opaque");
            m_RenderPassFullGBuffer.EndPass();

            // draw skydome
            {
                m_RenderPassJustDepthAndHdr.BeginPass(pCmdLst1, false);

                // Render skydome
                if (pState->SelectedSkydomeTypeIndex == 1)
                {
                    math::Matrix4 clipToView = math::inverse(pPerFrame.front()->mCameraCurrViewProj);
                    m_SkyDome.Draw(pCmdLst1, clipToView);
                    m_GPUTimer.GetTimeStamp(pCmdLst1, "Skydome cube");
                }
                else if (pState->SelectedSkydomeTypeIndex == 0)
                {
                    SkyDomeProc::Constants skyDomeConstants;
                    skyDomeConstants.invViewProj = math::inverse(pPerFrame.front()->mCameraCurrViewProj);
                    skyDomeConstants.vSunDirection = math::Vector4(1.0f, 0.05f, 0.0f, 0.0f);
                    skyDomeConstants.turbidity = 10.0f;
                    skyDomeConstants.rayleigh = 2.0f;
                    skyDomeConstants.mieCoefficient = 0.005f;
                    skyDomeConstants.mieDirectionalG = 0.8f;
                    skyDomeConstants.luminance = 1.0f;
                    m_SkyDomeProc.Draw(pCmdLst1, skyDomeConstants);

                    m_GPUTimer.GetTimeStamp(pCmdLst1, "Skydome proc");
                }

                m_RenderPassJustDepthAndHdr.EndPass();
            }
        }

        // Draw status boxes.
        {

            UserMarker marker(pCmdLst1, "Test box");

            auto& streamingVolumes(*m_pStreamingVolumes);
            math::Vector4 vColor = math::Vector4(1.0f, 0.0f, 1.0f, 0.2f);

            for (auto& vol : streamingVolumes)
            {
                if (vol.m_radius.getX() > 100) continue;

                math::Matrix4 worldMatrix = pPerFrame[0]->mCameraCurrViewProj;
                m_WireframeBox.Draw(pCmdLst1, &m_Wireframe, worldMatrix, math::Vector4(vol.m_center), math::Vector4(vol.m_radius), vColor);

                vColor.setW(0.25);

                switch (vol.m_LoadingState)
                {
                case GLTFLoadingState::Loaded:
                    vColor.setXYZ(Vectormath::Vector3(0,1,0));
                    break;
                case GLTFLoadingState::Loading:
                    vColor.setXYZ(Vectormath::Vector3(1,1,0));
                    break;
                case GLTFLoadingState::CancellationRequested:
                    vColor.setXYZ(Vectormath::Vector3(1,0,0));
                    break;
                case GLTFLoadingState::Unloading:
                    vColor.setXYZ(Vectormath::Vector3(1, 0.423, 0));
                    break;

                default:
                case GLTFLoadingState::Unloaded:
                    vColor.setXYZ(Vectormath::Vector3(0,0,0));
                    break;
                }

                m_TransparentCube.Draw(pCmdLst1, worldMatrix, math::Vector4(vol.m_center), math::Vector4(vol.m_radius), vColor);
            }
        }
    }

    D3D12_RESOURCE_BARRIER preResolve[1] = {
        CD3DX12_RESOURCE_BARRIER::Transition(m_GBuffer.m_HDR.GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
    };
    pCmdLst1->ResourceBarrier(1, preResolve);

    m_ArtificialWorkload.Draw(pCmdLst1, m_pSampleOptions->workloadOptions.m_mandelbrotIterations);


    // Post proc---------------------------------------------------------------------------

    // Bloom, takes HDR as input and applies bloom to it.
    {
        D3D12_CPU_DESCRIPTOR_HANDLE renderTargets[] = { m_GBuffer.m_HDRRTV.GetCPU() };
        pCmdLst1->OMSetRenderTargets(ARRAYSIZE(renderTargets), renderTargets, false, NULL);

        m_DownSample.Draw(pCmdLst1);
        m_GPUTimer.GetTimeStamp(pCmdLst1, "Downsample");

        m_Bloom.Draw(pCmdLst1, &m_GBuffer.m_HDR);
        m_GPUTimer.GetTimeStamp(pCmdLst1, "Bloom");
    }

    // Apply TAA & Sharpen to m_HDR
    if (pState->bUseTAA)
    {
        m_TAA.Draw(pCmdLst1, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_GPUTimer.GetTimeStamp(pCmdLst1, "TAA");
    }


    // Start tracking input/output resources at this point to handle HDR and SDR render paths 
    ID3D12Resource*              pRscCurrentInput = m_GBuffer.m_HDR.GetResource();
    CBV_SRV_UAV                  SRVCurrentInput  = m_GBuffer.m_HDRSRV;
    D3D12_CPU_DESCRIPTOR_HANDLE  RTVCurrentOutput = m_GBuffer.m_HDRRTV.GetCPU();
    CBV_SRV_UAV                  UAVCurrentOutput = m_GBuffer.m_HDRUAV;
    

    // If using FreeSync HDR we need to do the tonemapping in-place and then apply the GUI, later we'll apply the color conversion into the swapchain
    const bool bHDR = pSwapChain->GetDisplayMode() != DISPLAYMODE_SDR;
    if (bHDR)
    {
        // In place Tonemapping ------------------------------------------------------------------------
        {
            D3D12_RESOURCE_BARRIER inputRscToUAV = CD3DX12_RESOURCE_BARRIER::Transition(pRscCurrentInput, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            pCmdLst1->ResourceBarrier(1, &inputRscToUAV);

            m_ToneMappingCS.Draw(pCmdLst1, &UAVCurrentOutput, pState->Exposure, pState->SelectedTonemapperIndex, m_Width, m_Height);

            D3D12_RESOURCE_BARRIER inputRscToRTV = CD3DX12_RESOURCE_BARRIER::Transition(pRscCurrentInput, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);
            pCmdLst1->ResourceBarrier(1, &inputRscToRTV);
        }

        // Render HUD  ------------------------------------------------------------------------
        {
            pCmdLst1->RSSetViewports(1, &m_Viewport);
            pCmdLst1->RSSetScissorRects(1, &m_RectScissor);
            pCmdLst1->OMSetRenderTargets(1, &RTVCurrentOutput, true, NULL);

            m_ImGUI.Draw(pCmdLst1);

            D3D12_RESOURCE_BARRIER hdrToSRV = CD3DX12_RESOURCE_BARRIER::Transition(pRscCurrentInput, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            pCmdLst1->ResourceBarrier(1, &hdrToSRV);

            m_GPUTimer.GetTimeStamp(pCmdLst1, "ImGUI Rendering");
        }
    }

    pCmdLst1->RSSetViewports(1, &m_Viewport);
    pCmdLst1->RSSetScissorRects(1, &m_RectScissor);
    pCmdLst1->OMSetRenderTargets(1, pSwapChain->GetCurrentBackBufferRTV(), true, NULL);

    if (bHDR)
    {
        // FS HDR mode! Apply color conversion now.
        m_ColorConversionPS.Draw(pCmdLst1, &SRVCurrentInput);
        m_GPUTimer.GetTimeStamp(pCmdLst1, "Color conversion");

        pCmdLst1->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pRscCurrentInput, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
    }
    else
    {
        // non FreeSync HDR mode, that is SDR, here we apply the tonemapping from the HDR into the swapchain and then we render the GUI

        // Tonemapping ------------------------------------------------------------------------
        {
            m_ToneMappingPS.Draw(pCmdLst1, &SRVCurrentInput, pState->Exposure, pState->SelectedTonemapperIndex);
            m_GPUTimer.GetTimeStamp(pCmdLst1, "Tone mapping");

            pCmdLst1->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pRscCurrentInput, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
        }

        // Render HUD  ------------------------------------------------------------------------
        {
            m_ImGUI.Draw(pCmdLst1);
            m_GPUTimer.GetTimeStamp(pCmdLst1, "ImGUI Rendering");
        }
    }

    // Transition swapchain into present mode
    pCmdLst1->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pSwapChain->GetCurrentBackBufferResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));


    m_GPUTimer.CollectTimings(pCmdLst1);

    m_GPUTimer.OnEndFrame();


    // Close & Submit the command list #2 -------------------------------------------------
    ThrowIfFailed(pCmdLst1->Close());

    // Wait for swapchain (we are going to render to it) -----------------------------------
    pSwapChain->WaitForSwapChain(); // technically, this could be done on the GPU, and the CPU wait could occur at the beginning of the next frame.

    ID3D12CommandList* CmdListLists[] = { pCmdLst1 };
    m_pDevice->GetGraphicsQueue()->ExecuteCommandLists(1, CmdListLists);
}

    std::future<SceneData*> Renderer::UnloadSceneAsync(std::string sceneName, SceneData* sceneData)
    {
        Trace("U Load: %s", sceneName.c_str());
        return std::async([this,sceneData]()
            {
                this->m_pDevice->GPUFlush();
                sceneData->OnDestroy();
                return static_cast<SceneData*>(nullptr);
            }
        );
    }

    std::future<SceneData*> Renderer::LoadSceneAsync(const SceneLoadRequest& loadRequest)
    {
        if (loadRequest.m_useDirectStorage)
        {
            return LoadSceneAsyncDirectStorage(loadRequest);
        }
        else
        {
            return LoadSceneAsyncNoDirectStorage(loadRequest);
        }
    }

    std::future<SceneData*> Renderer::LoadSceneAsyncDirectStorage(const SceneLoadRequest& loadRequest)
    {
        CPUUserMarker marker("LoadSceneAsyncDirectStorage Requested");

        double requestTime = MillisecondsNow();
        auto frameTimeMeanAtRequestTime = m_rollingFrameTimeSnapshot.GetArithmeticMean();
        auto frameTimeMedianAtRequestTime = m_rollingFrameTimeSnapshot.GetMedian();

        return std::async([this, loadRequest, requestTime, frameTimeMeanAtRequestTime, frameTimeMedianAtRequestTime]()
            {
                CPUUserMarker marker("LoadSceneAsync Processing");

        Trace("S Load: %s", loadRequest.m_sceneName->c_str());

        const auto& scenePathLookupResult = m_pScenePathMap->at(*loadRequest.m_sceneName);
        AsyncPool asyncPool;
        SceneData* sceneData = new SceneData;

        uint64_t fenceId = 0;
        sceneData->m_timingData.loadRequest = loadRequest;
        sceneData->m_timingData.frameTimeMeanBeforeLoading = frameTimeMeanAtRequestTime;
        sceneData->m_timingData.frameTimeMedianBeforeLoading = frameTimeMedianAtRequestTime;
        sceneData->m_timingData.sceneTextureFileDataSize = Sample::GetSceneTextureDataSizeOnDisk(scenePathLookupResult);
        sceneData->m_timingData.sceneTextureUncompressedSize = Sample::GetSceneTextureDataSizeUncompressed(scenePathLookupResult);
        sceneData->m_timingData.sceneTextureCompressionRatio = Sample::GetSceneTextureCompressionRatio(scenePathLookupResult);

        if (sceneData->m_gltfCommon.Load(scenePathLookupResult.scenePath, scenePathLookupResult.sceneFile) == false)
        {
            MessageBox(NULL, "The selected model couldn't be found, please check the documentation", "Cauldron Panic!", MB_ICONERROR);
            exit(0);
        }

        sceneData->m_pTexturesAndBuffers = reinterpret_cast<CAULDRON_DX12::GLTFTexturesAndBuffers*>(new Sample::GLTFTexturesAndBuffers);
   
        // Create a 'static' pool for vertices, indices and constant buffers... should be using placed resources here too. Value is based on assets being loaded.
        const size_t staticGeometryMemSize = (240 * 1024 * 1024) - (256 * 1024);

        sceneData->m_staticBufferPool.OnCreate(m_pDevice, staticGeometryMemSize, true, "StaticGeom");

        // our own constant buffer ring... this might not be necessary.
        // Create a 'dynamic' constant buffer
        const uint32_t constantBuffersMemSize = 2 * 1024 * 1024;
        sceneData->m_constantBuffer.OnCreate(m_pDevice, backBufferCount, constantBuffersMemSize);

        // Use placed resources if requested and compatible for this asset.
        sceneData->m_pTextureHeap = nullptr;
        if (loadRequest.m_usePlacedResources)
        {
            D3D12_HEAP_DESC heapDesc = Sample::GetTextureHeapDescForScene(scenePathLookupResult);
            m_pDevice->GetDevice()->CreateHeap(&heapDesc, IID_PPV_ARGS(&sceneData->m_pTextureHeap));
        }

        sceneData->m_pTexturesAndBuffers->OnCreate(m_pDevice, &sceneData->m_gltfCommon, nullptr, &sceneData->m_staticBufferPool, &sceneData->m_constantBuffer);

        reinterpret_cast<Sample::GLTFTexturesAndBuffers*>(sceneData->m_pTexturesAndBuffers)->LoadTextures(&asyncPool, sceneData->m_pTextureHeap, loadRequest.workloadId);
        Sample::DStorageEndProfileLoading(loadRequest.workloadId);
        fenceId = Sample::DStorageInsertFenceCPU();
        Sample::DStorageSubmit();

        // Create all the heaps for the resources views. @todo: these could easily be sized to the data.
        const uint32_t cbvDescriptorCount = 4000;
        const uint32_t srvDescriptorCount = 8000;
        const uint32_t uavDescriptorCount = 10;
        const uint32_t dsvDescriptorCount = 10;
        const uint32_t rtvDescriptorCount = 60;
        const uint32_t samplerDescriptorCount = 20;

        sceneData->m_resourceViewHeaps.OnCreate(m_pDevice, cbvDescriptorCount, srvDescriptorCount, uavDescriptorCount, dsvDescriptorCount, rtvDescriptorCount, samplerDescriptorCount);

        // do the rest of the pass creation.
        {
            CPUUserMarker p("Create passes");

            {
                CPUUserMarker p("m_GLTFPBR->OnCreate");

                // same thing as above but for the PBR pass
                sceneData->m_pbrPass.OnCreate(
                    m_pDevice,
                    nullptr,
                    &sceneData->m_resourceViewHeaps,
                    &sceneData->m_constantBuffer,
                    sceneData->m_pTexturesAndBuffers,
                    &m_SkyDome,
                    &m_BRDFLUT,
                    false,                  // use a SSAO mask
                    ShadowReceivingState_None,
                    &m_RenderPassFullGBuffer,
                    &asyncPool
                );
            }
        }

       
        // We did this because we no longer required our own upload heap when using DirectStorage, but geometry didn't use DS.
        ID3D12CommandAllocator* pCommandAllocator = nullptr;
        ID3D12GraphicsCommandList* pCommandList = nullptr;
        m_pDevice->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pCommandAllocator));
        m_pDevice->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocator, nullptr, IID_PPV_ARGS(&pCommandList));

        sceneData->m_staticBufferPool.UploadData(pCommandList);
        pCommandList->Close();
        m_pDevice->GetGraphicsQueue()->ExecuteCommandLists(1, CommandListCast(&pCommandList));

        HANDLE evt = CreateEvent(0, 0, 0, 0);
        ID3D12Fence* fence = nullptr;
        m_pDevice->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        m_pDevice->GetGraphicsQueue()->Signal(fence, 1);
        fence->SetEventOnCompletion(1, evt);
        WaitForSingleObject(evt, INFINITE);
        fence->Release();
        CloseHandle(evt);


        Sample::DStorageSyncCPU(fenceId);
        //Sample::DStorageSyncGPU(m_pDevice->GetGraphicsQueue());

        sceneData->m_sceneTransform = loadRequest.m_streamedSceneDataTransform;
        sceneData->m_timingData.loadTime = MillisecondsNow() - requestTime;

        Trace("E Load: %s", loadRequest.m_sceneName->c_str());

        return sceneData;
            });
    }

    std::future<SceneData*> Renderer::LoadSceneAsyncNoDirectStorage(const SceneLoadRequest& loadRequest)
    {
        CPUUserMarker marker("LoadSceneAsyncNoDirectStorage Requested");

        double requestTime = MillisecondsNow();
        auto frameTimeMeanAtRequestTime = m_rollingFrameTimeSnapshot.GetArithmeticMean();
        auto frameTimeMedianAtRequestTime = m_rollingFrameTimeSnapshot.GetMedian();

        return std::async([this, loadRequest, requestTime, frameTimeMeanAtRequestTime, frameTimeMedianAtRequestTime]()
            {
                CPUUserMarker marker("LoadSceneAsync Processing");

        Trace("S Load: %s", loadRequest.m_sceneName->c_str());

        const auto& scenePathLookupResult = m_pScenePathMap->at(*loadRequest.m_sceneName);
        AsyncPool asyncPool;
        SceneData* sceneData = new SceneData;

        sceneData->m_timingData.loadRequest = loadRequest;
        sceneData->m_timingData.frameTimeMeanBeforeLoading = frameTimeMeanAtRequestTime;
        sceneData->m_timingData.frameTimeMedianBeforeLoading = frameTimeMedianAtRequestTime;

        if (sceneData->m_gltfCommon.Load(scenePathLookupResult.scenePath, scenePathLookupResult.sceneFile) == false)
        {
            MessageBox(NULL, "The selected model couldn't be found, please check the documentation", "Cauldron Panic!", MB_ICONERROR);
            exit(0);
        }

        sceneData->m_pTexturesAndBuffers = new CAULDRON_DX12::GLTFTexturesAndBuffers;

        // Create a temporary upload heap used when not using DS.
        UploadHeap uploadHeap;

        // Quick helper to upload resources, it has it's own commandList and uses suballocation. It's really only used without DirectStorage.
        uint32_t uploadHeapSize = 512 * 1024 * 1024;

        uploadHeap.OnCreate(m_pDevice, uploadHeapSize);


        // Create a 'static' pool for vertices, indices and constant buffers... should be using placed resources here too. Value is based on assets being loaded.
        const size_t staticGeometryMemSize = (240 * 1024 * 1024) - (256 * 1024);

        sceneData->m_staticBufferPool.OnCreate(m_pDevice, staticGeometryMemSize, true, "StaticGeom");

        // our own constant buffer ring... this might not be necessary.
        // Create a 'dynamic' constant buffer
        const uint32_t constantBuffersMemSize = 2 * 1024 * 1024;
        sceneData->m_constantBuffer.OnCreate(m_pDevice, backBufferCount, constantBuffersMemSize);

        // Use placed resources if requested and compatible for this asset.
        sceneData->m_pTextureHeap = nullptr;

        sceneData->m_pTexturesAndBuffers->OnCreate(m_pDevice, &sceneData->m_gltfCommon, &uploadHeap, &sceneData->m_staticBufferPool, &sceneData->m_constantBuffer);

        // Create all the heaps for the resources views. @todo: these could easily be sized to the data.
        const uint32_t cbvDescriptorCount = 4000;
        const uint32_t srvDescriptorCount = 8000;
        const uint32_t uavDescriptorCount = 10;
        const uint32_t dsvDescriptorCount = 10;
        const uint32_t rtvDescriptorCount = 60;
        const uint32_t samplerDescriptorCount = 20;

        sceneData->m_resourceViewHeaps.OnCreate(m_pDevice, cbvDescriptorCount, srvDescriptorCount, uavDescriptorCount, dsvDescriptorCount, rtvDescriptorCount, samplerDescriptorCount);


        reinterpret_cast<CAULDRON_DX12::GLTFTexturesAndBuffers*>(sceneData->m_pTexturesAndBuffers)->LoadTextures(&asyncPool, sceneData->m_pTextureHeap, loadRequest.workloadId);


        // do the rest of the pass creation.
        {
            CPUUserMarker p("Create passes");

            {
                CPUUserMarker p("m_GLTFPBR->OnCreate");

                // same thing as above but for the PBR pass
                sceneData->m_pbrPass.OnCreate(
                    m_pDevice,
                    &uploadHeap,
                    &sceneData->m_resourceViewHeaps,
                    &sceneData->m_constantBuffer,
                    sceneData->m_pTexturesAndBuffers,
                    &m_SkyDome,
                    &m_BRDFLUT,
                    false,                  // use a SSAO mask
                    ShadowReceivingState_None,
                    &m_RenderPassFullGBuffer,
                    &asyncPool
                );
            }
        }


        sceneData->m_staticBufferPool.UploadData(uploadHeap.GetCommandList());
        uploadHeap.FlushAndFinish();
        uploadHeap.OnDestroy();

        sceneData->m_sceneTransform = loadRequest.m_streamedSceneDataTransform;
        sceneData->m_timingData.loadTime = MillisecondsNow() - requestTime;

        Trace("E Load: %s", loadRequest.m_sceneName->c_str());

        return sceneData;
            });
    }

    bool StreamingVolume::IsPointInside(const math::Point3& point) const
    {
        math::Point3 maxs =  m_center + m_radius;
        math::Point3 mins = m_center - m_radius;

        auto lessEqualThanMaxMask = _mm_cmple_ps(point.get128(), maxs.get128());
        auto greaterEqualThanMinsMask = _mm_cmpge_ps(point.get128(), mins.get128());

        auto andMask = _mm_and_ps(lessEqualThanMaxMask, greaterEqualThanMinsMask);
        auto compareResult = _mm_movemask_ps(andMask);

        return compareResult == 0x7;
    }
