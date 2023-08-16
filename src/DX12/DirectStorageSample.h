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

#include "base/FrameworkWindows.h"
#include "Renderer.h"
#include "UI.h"
#include "Profile.h"
#include "SampleOptions.h"

class DirectStorageSample : public FrameworkWindows
{
public:
    DirectStorageSample(LPCSTR name);
    void OnParseCommandLine(LPSTR lpCmdLine, uint32_t* pWidth, uint32_t* pHeight) override;
    void OnCreate() override;
    void OnDestroy() override;
    void OnRender() override;
    bool OnEvent(MSG msg) override;
    void OnResize(bool resizeRender) override;
    void OnUpdateDisplay() override;

    void BuildUI();
    
    void CheckAndRequestScenesToLoad();
    void ShutdownStreaming();

    void OnUpdate();

    void HandleInput(const ImGuiIO& io);
    void UpdateCamera(Camera& cam, const ImGuiIO* io);
    
private:
    SampleOptions               m_sampleOptions;
    AsyncPool                   m_AsyncPool;

    Renderer*                   m_pRenderer = NULL;
    UIState                     m_UIState;
    float                       m_fontSize;
    Camera                      m_camera;

    bool                        m_bioTiming = false;
    bool                        m_bProfilerOutputEnabled = false;
    std::string                 m_profilerOutputPath{"profilerOutput.csv"};
    Sample::Profiler            m_profiler;
    float                       m_profileSeconds = 0;
    bool                        m_disableGPUDecompression = false;
    bool                        m_disableMetaCommand = false;

    float                       m_time; // Time accumulator in seconds, used for animation.

    // json config file
    json                        m_jsonConfigFile;
    std::vector<std::string>    m_sceneNames;
    std::unordered_map<std::string, ScenePathPair> m_sceneNameToScenePath;
    std::vector<StreamingVolume> m_StreamingVolumes;

    bool                        m_bPlay;
};
