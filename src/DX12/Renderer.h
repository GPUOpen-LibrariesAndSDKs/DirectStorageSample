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

#include "stdafx.h"

#include "base/GBuffer.h"
#include "ArtificialWorkload.h"
#include "TransparentCube.h"

struct UIState;

// We are queuing (backBufferCount + 0.5) frames, so we need to triple buffer the resources that get modified each frame
static const int backBufferCount = 3;

#define USE_SHADOWMASK false

using namespace CAULDRON_DX12;

namespace Sample { class GLTFTexturesAndBuffers; };


struct ScenePathPair
{
    std::string scenePath;
    std::string sceneFile;
};


struct FrameTimeSnapshotIFace
{
    virtual void AddTime(int64_t timeMicroseconds) = 0;
    virtual double GetMedian() = 0;
    virtual double GetArithmeticMean() = 0;
    virtual void Reset() = 0;
};

struct FrameTimeSnapshotUnlimited final : public FrameTimeSnapshotIFace
{
    FrameTimeSnapshotUnlimited() = default;

    void AddTime(int64_t timeMicroseconds) override
    {
        frameTimes.push_back(timeMicroseconds);
    }

    double GetMedian() override
    {
        std::sort(frameTimes.begin(), frameTimes.end());
        return frameTimes[frameTimes.size() / 2];
    }

    double GetArithmeticMean() override
    {
        decltype(frameTimes)::value_type defaultValue{};
        auto sum = std::accumulate(frameTimes.begin(), frameTimes.end(), defaultValue);

        if (frameTimes.size() > 0)
            return sum / static_cast<double>(frameTimes.size());

        return defaultValue;
    }

    uint64_t GetPopulationCount() const
    {
        return frameTimes.size();
    }

    void Reset() override
    {
        frameTimes.clear();
    }

    std::deque<int64_t> frameTimes;
};


struct FrameTimeSnapshotRolling final : public FrameTimeSnapshotIFace
{
    FrameTimeSnapshotRolling() = default;

    void AddTime(int64_t timeMicroseconds) override
    {
        frameTimes[frameIdx] = timeMicroseconds;
        frameIdx = (frameIdx + 1) % frameTimes.size();
        hasFilledOnce = frameIdx == 0 ? true : false;
    }

    double GetMedian() override
    {
        frameTimesCopy = frameTimes;
        auto endIter = hasFilledOnce ? frameTimesCopy.end() : (frameTimesCopy.begin() + frameIdx);
        auto medianIdx = std::distance(frameTimesCopy.begin(), endIter) / 2;
        std::sort(frameTimesCopy.begin(), hasFilledOnce ? frameTimesCopy.end() : endIter);

        return frameTimesCopy[medianIdx];
    }

    double GetArithmeticMean() override
    {
        auto endIter = hasFilledOnce ? frameTimes.end() : (frameTimes.begin() + frameIdx);
        auto length = std::distance(frameTimes.begin(), endIter);

        decltype(frameTimes)::value_type defaultValue{};
        auto sum = std::accumulate(frameTimes.begin(), endIter, defaultValue);

        return length > 0 ? sum / static_cast<double>(length) : defaultValue;
    }

    void Reset() override
    {
        *this = FrameTimeSnapshotRolling();
    }

    bool hasFilledOnce = false;
    uint32_t frameIdx{ 0 };
    std::array<int64_t, 60> frameTimes;
    std::array<int64_t, 60> frameTimesCopy;
};

struct SceneLoadRequest
{
    uint64_t workloadId{ 0 };
    std::string* m_sceneName{ nullptr };
    math::Matrix4 m_streamedSceneDataTransform{ math::Matrix4::identity() };
    bool m_useDirectStorage{ false };
    bool m_usePlacedResources{ false };
};

struct SceneTimingData
{
    SceneLoadRequest loadRequest;
    double loadTime{0};
    double frameTimeMeanBeforeLoading = 0.0;
    double frameTimeMedianBeforeLoading = 0.0;
    double ioTime = 0.0;
    double sceneTextureCompressionRatio = 0;
    size_t sceneTextureFileDataSize = 0;
    size_t sceneTextureUncompressedSize = 0;
    FrameTimeSnapshotUnlimited frameTimes;
};

enum class GLTFLoadingState
{
    Loaded, // Ready to rock.
    Loading,
    CancellationRequested,
    Unloading,
    Unloaded
};

struct SceneData
{
    ID3D12Heap* m_pTextureHeap;
    
    // Data held by passes but owned here.
    GLTFCommon m_gltfCommon;
    CAULDRON_DX12::GLTFTexturesAndBuffers* m_pTexturesAndBuffers;
    StaticBufferPool m_staticBufferPool;
    DynamicBufferRing m_constantBuffer;
    ResourceViewHeaps m_resourceViewHeaps;
    GltfPbrPass m_pbrPass;

    math::Matrix4 m_sceneTransform;

    SceneTimingData m_timingData;

    void OnDestroy()
    {
        m_pbrPass.OnDestroy();
        m_staticBufferPool.OnDestroy();
        m_constantBuffer.OnDestroy();
        m_resourceViewHeaps.OnDestroy();
        m_pTexturesAndBuffers->OnDestroy(); // TODO: Go in and ensure no references to the heap.
        m_pTexturesAndBuffers = nullptr;
        m_gltfCommon.Unload();
        if (m_pTextureHeap)
        {
            // for placed resources heap.
            auto textureHeapRefCount = m_pTextureHeap->Release();
            assert(textureHeapRefCount == 0);
            m_pTextureHeap = nullptr;
        }
    }
};

struct TimeState
{
    float m_deltaTime;
    float m_animationTime;
};

struct StreamingVolume
{
    math::Point3  m_center{0.0f,0.0f,0.0f}; // position to center.
    math::Vector3 m_radius{0.5f,0.5f,0.5f}; // distance from center to sides.
    math::Matrix4 m_streamedSceneDataTransform{ math::Matrix4::identity() }; // location/scale etc to place the streamed data.

    GLTFLoadingState m_LoadingState{ GLTFLoadingState::Unloaded };
    std::future<SceneData*> m_LoadedSceneFuture;
    FrameTimeSnapshotUnlimited frameTimes; // measure of frame time while loading this scene data.
    uint64_t workloadId = 0;
    SceneData* m_pSceneData;
    std::string m_sceneName;
    bool IsPointInside(const math::Point3& point) const;
};

// This class encapsulates the 'application' and is responsible for handling window events and scene updates (simulation)
// Rendering and rendering resource management is done by the Renderer class
//
// Renderer class is responsible for rendering resources management and recording command buffers.
class Renderer
{
public:
    void OnCreate(Device* pDevice, SwapChain* pSwapChain, AsyncPool* pAsyncPool, float FontSize, const std::unordered_map<std::string, ScenePathPair>* scenePathMap, std::vector<StreamingVolume>* streamingVolumes, const class SampleOptions* const sampleOptions);
    void OnDestroy();

    void OnCreateWindowSizeDependentResources(SwapChain* pSwapChain, uint32_t Width, uint32_t Height);
    void OnDestroyWindowSizeDependentResources();

    void OnUpdateDisplayDependentResources(SwapChain* pSwapChain);

    const std::vector<SceneTimingData const *> GetSceneLoadingTimings() const;
    const std::vector<TimeStamp>& GetTimingValues() const { return m_TimeStamps; }
    const float GetLastFrameTiming() const { return m_TimeStamps.size() > 0 ? m_TimeStamps.back().m_microseconds : 0.0; }

    void OnRender(const UIState* pState, const Camera& Cam, const TimeState& timeState, SwapChain* pSwapChain);

    std::future<SceneData*> LoadSceneAsync(const SceneLoadRequest& loadRequest);
    std::future<SceneData*> LoadSceneAsyncDirectStorage(const SceneLoadRequest& loadRequest);
    std::future<SceneData*> LoadSceneAsyncNoDirectStorage(const SceneLoadRequest& loadRequest);
    std::future<SceneData*> UnloadSceneAsync(std::string sceneName, SceneData* sceneData);

    void AddScene(SceneData* sceneData)
    {
        m_SceneData.push_back(sceneData);
    }
    void RemoveScene(SceneData* sceneData)
    {
        auto foundSceneItr = std::find(m_SceneData.begin(), m_SceneData.end(), sceneData);
        if (foundSceneItr != m_SceneData.end())
        {
            m_SceneData.erase(foundSceneItr);
        }
    }
    
private:
    Device                         *m_pDevice;
    const SampleOptions            *m_pSampleOptions;

    uint32_t                        m_Width;
    uint32_t                        m_Height;
    D3D12_VIEWPORT                  m_Viewport;
    D3D12_RECT                      m_RectScissor;

    // Initialize helper classes
    ResourceViewHeaps               m_ResourceViewHeaps;
    UploadHeap                      m_UploadHeap;
    DynamicBufferRing               m_ConstantBufferRing;
    StaticBufferPool                m_VidMemBufferPool;
    CommandListRing                 m_CommandListRing;
    GPUTimestamps                   m_GPUTimer;

    // Scenedata for streamed assets.
    const std::unordered_map<std::string, ScenePathPair>* m_pScenePathMap{ nullptr };
    std::vector<StreamingVolume>* m_pStreamingVolumes{ nullptr };
    std::vector<SceneData*> m_SceneData;
    FrameTimeSnapshotRolling        m_rollingFrameTimeSnapshot;

    Texture                         m_BRDFLUT;

    // effects
    Bloom                           m_Bloom;
    SkyDome                         m_SkyDome;
    DownSamplePS                    m_DownSample;
    SkyDomeProc                     m_SkyDomeProc;
    ToneMapping                     m_ToneMappingPS;
    ToneMappingCS                   m_ToneMappingCS;
    ColorConversionPS               m_ColorConversionPS;
    TAA                             m_TAA;

    // Artificial workload.
    ArtificialWorkload              m_ArtificialWorkload;

    // Transparent status cube.
    TransparentCube                 m_TransparentCube;

    // GUI
    ImGUI                           m_ImGUI;

    // Temporary render targets
    GBuffer                         m_GBuffer;
    GBufferRenderPass               m_RenderPassFullGBuffer;
    GBufferRenderPass               m_RenderPassJustDepthAndHdr;

    // widgets
    Wireframe                       m_Wireframe;
    WireframeBox                    m_WireframeBox;

    std::vector<TimeStamp>          m_TimeStamps;

    AsyncPool*                      m_pAsyncPool;
};
