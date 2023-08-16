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
#include "UI.h"
#include "DirectStorageSample.h"
#include "imgui.h"
#include "base/FrameworkWindows.h"

// To use the 'disabled UI state' functionality (ImGuiItemFlags_Disabled), include internal header
// https://github.com/ocornut/imgui/issues/211#issuecomment-339241929
#include "imgui_internal.h"
static void DisableUIStateBegin(const bool& bEnable)
{
    if (!bEnable)
    {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
};
static void DisableUIStateEnd(const bool& bEnable)
{
    if (!bEnable)
    {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
};

// Some constants and utility functions
template<class T> static T clamped(const T& v, const T& min, const T& max)
{
    if (v < min)      return min;
    else if (v > max) return max;
    else              return v;
}


void DirectStorageSample::BuildUI()
{
    // if we haven't initialized GLTFLoader yet, don't draw UI.
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameBorderSize = 1.0f;

    const uint32_t W = this->GetWidth();
    const uint32_t H = this->GetHeight();

    const uint32_t PROFILER_WINDOW_PADDIG_X = 10;
    const uint32_t PROFILER_WINDOW_PADDIG_Y = 10;
    const uint32_t PROFILER_WINDOW_SIZE_X = 480;
    const uint32_t PROFILER_WINDOW_SIZE_Y = H - (PROFILER_WINDOW_PADDIG_Y*2);
    const uint32_t PROFILER_WINDOW_POS_X = W - PROFILER_WINDOW_PADDIG_X - PROFILER_WINDOW_SIZE_X;
    const uint32_t PROFILER_WINDOW_POS_Y = PROFILER_WINDOW_PADDIG_Y;

    const uint32_t CONTROLS_WINDOW_POS_X = 10;
    const uint32_t CONTROLS_WINDOW_POS_Y = 10;
    const uint32_t CONTROLW_WINDOW_SIZE_X = 640;
    const uint32_t CONTROLW_WINDOW_SIZE_Y = 480; // assuming > 720p

    // Legend
    const uint32_t LEGEND_WINDOW_PAD = 10;
    const uint32_t LEGEND_WINDOW_SIZE_X = 200;
    const uint32_t LEGEND_WINDOW_SIZE_Y = 375;
    const uint32_t LEGEND_WINDOW_POS_X = LEGEND_WINDOW_PAD;
    const uint32_t LEGEND_WINDOW_POS_Y = H - LEGEND_WINDOW_SIZE_Y - LEGEND_WINDOW_PAD;

    // Render CONTROLS window
    //
    ImGui::SetNextWindowPos(ImVec2(CONTROLS_WINDOW_POS_X, CONTROLS_WINDOW_POS_Y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(CONTROLW_WINDOW_SIZE_X, CONTROLW_WINDOW_SIZE_Y), ImGuiCond_FirstUseEver);

    if (m_UIState.bShowControlsWindow)
    {
        ImGui::Begin("CONTROLS (F1)", &m_UIState.bShowControlsWindow);
        if (ImGui::CollapsingHeader("Animation", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Autopilot", &m_UIState.bAutopilot);
            ImGui::Checkbox("Play", &m_bPlay);
            ImGui::SliderFloat("Speed", &m_sampleOptions.cameraOptions.m_cameraSpeed, 0.0f, 20.0f);
        }

        ImGui::Spacing();
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Artificial Workload", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SliderInt("Mandelbrot Iterations",  (int32_t*)& m_sampleOptions.workloadOptions.m_mandelbrotIterations, 0, 500000);
        }

        ImGui::Spacing();
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Rendering", 0))
        {
            ImGui::SliderFloat("Emissive Intensity", &m_UIState.EmissiveFactor, 1.0f, 1000.0f, NULL, 1.0f);

            const char* skyDomeType[] = { "Procedural Sky", "Environment Map", "Clear" };
            ImGui::Combo("Skydome", &m_UIState.SelectedSkydomeTypeIndex, skyDomeType, _countof(skyDomeType));

            ImGui::SliderFloat("IBL Factor", &m_UIState.IBLFactor, 0.0f, 3.0f);

            ImGui::Spacing();
            ImGui::Spacing();

            if (ImGui::CollapsingHeader("PostProcessing", ImGuiTreeNodeFlags_DefaultOpen))
            {
                const char* tonemappers[] = { "AMD Tonemapper", "DX11DSK", "Reinhard", "Uncharted2Tonemap", "ACES", "No tonemapper" };
                ImGui::Combo("Tonemapper", &m_UIState.SelectedTonemapperIndex, tonemappers, _countof(tonemappers));

                ImGui::SliderFloat("Exposure", &m_UIState.Exposure, 0.0f, 4.0f);

                ImGui::Checkbox("TAA", &m_UIState.bUseTAA);
            }

            ImGui::Spacing();
            ImGui::Spacing();

            if (ImGui::CollapsingHeader("Debug", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Show Bounding Boxes", &m_UIState.bDrawBoundingBoxes);
                ImGui::Checkbox("Show Light Frustum", &m_UIState.bDrawLightFrustum);

                ImGui::Text("Wireframe");
                ImGui::SameLine(); ImGui::RadioButton("Off", (int*)&m_UIState.WireframeMode, (int)UIState::WireframeMode::WIREFRAME_MODE_OFF);
                ImGui::SameLine(); ImGui::RadioButton("Shaded", (int*)&m_UIState.WireframeMode, (int)UIState::WireframeMode::WIREFRAME_MODE_SHADED);
                ImGui::SameLine(); ImGui::RadioButton("Solid color", (int*)&m_UIState.WireframeMode, (int)UIState::WireframeMode::WIREFRAME_MODE_SOLID_COLOR);
                if (m_UIState.WireframeMode == UIState::WireframeMode::WIREFRAME_MODE_SOLID_COLOR)
                    ImGui::ColorEdit3("Wire solid color", m_UIState.WireframeColor, ImGuiColorEditFlags_NoAlpha);
            }


            ImGui::Spacing();
            ImGui::Spacing();

            if (ImGui::CollapsingHeader("Presentation Mode", ImGuiTreeNodeFlags_DefaultOpen))
            {
                const char* fullscreenModes[] = { "Windowed", "BorderlessFullscreen", "ExclusiveFulscreen" };
                if (ImGui::Combo("Fullscreen Mode", (int*)&m_fullscreenMode, fullscreenModes, _countof(fullscreenModes)))
                {
                    if (m_previousFullscreenMode != m_fullscreenMode)
                    {
                        HandleFullScreen();
                        m_previousFullscreenMode = m_fullscreenMode;
                    }
                }
            }

            ImGui::Spacing();
            ImGui::Spacing();

            if (m_FreesyncHDROptionEnabled && ImGui::CollapsingHeader("FreeSync HDR", ImGuiTreeNodeFlags_DefaultOpen))
            {
                static bool openWarning = false;
                const char** displayModeNames = &m_displayModesNamesAvailable[0];
                if (ImGui::Combo("Display Mode", (int*)&m_currentDisplayModeNamesIndex, displayModeNames, (int)m_displayModesNamesAvailable.size()))
                {
                    if (m_fullscreenMode != PRESENTATIONMODE_WINDOWED)
                    {
                        UpdateDisplay(m_disableLocalDimming);
                        m_previousDisplayModeNamesIndex = m_currentDisplayModeNamesIndex;
                    }
                    else if (CheckIfWindowModeHdrOn() &&
                        (m_displayModesAvailable[m_currentDisplayModeNamesIndex] == DISPLAYMODE_SDR ||
                            m_displayModesAvailable[m_currentDisplayModeNamesIndex] == DISPLAYMODE_HDR10_2084 ||
                            m_displayModesAvailable[m_currentDisplayModeNamesIndex] == DISPLAYMODE_HDR10_SCRGB))
                    {
                        UpdateDisplay(m_disableLocalDimming);
                        m_previousDisplayModeNamesIndex = m_currentDisplayModeNamesIndex;
                    }
                    else
                    {
                        openWarning = true;
                        m_currentDisplayModeNamesIndex = m_previousDisplayModeNamesIndex;
                    }
                }

                if (openWarning)
                {
                    ImGui::OpenPopup("Display Modes Warning");
                    ImGui::BeginPopupModal("Display Modes Warning", NULL, ImGuiWindowFlags_AlwaysAutoResize);
                    ImGui::Text("\nChanging display modes is only available either using HDR toggle in windows display setting for HDR10 modes or in fullscreen for FS HDR modes\n\n");
                    if (ImGui::Button("Cancel", ImVec2(120, 0))) { openWarning = false; ImGui::CloseCurrentPopup(); }
                    ImGui::EndPopup();
                }

                if (m_displayModesAvailable[m_currentDisplayModeNamesIndex] == DisplayMode::DISPLAYMODE_FSHDR_Gamma22 ||
                    m_displayModesAvailable[m_currentDisplayModeNamesIndex] == DisplayMode::DISPLAYMODE_FSHDR_SCRGB)
                {
                    static bool selectedDisableLocaldimmingSetting = false;
                    if (ImGui::Checkbox("Disable Local Dimming", &selectedDisableLocaldimmingSetting))
                        UpdateDisplay(selectedDisableLocaldimmingSetting);
                }
            }
        }

        ImGui::End(); // CONTROLS
    }


    // Render PROFILER window
    //
    if (m_UIState.bShowProfilerWindow)
    {
        constexpr size_t NUM_FRAMES = 128;
        static float FRAME_TIME_ARRAY[NUM_FRAMES] = { 0 };

        // track highest frame rate and determine the max value of the graph based on the measured highest value
        static float RECENT_HIGHEST_FRAME_TIME = 0.0f;
        constexpr int FRAME_TIME_GRAPH_MAX_FPS[] = {   30  };
        static float  FRAME_TIME_GRAPH_MAX_VALUES[_countof(FRAME_TIME_GRAPH_MAX_FPS)] = { 0 }; // us
        for (int i = 0; i < _countof(FRAME_TIME_GRAPH_MAX_FPS); ++i) { FRAME_TIME_GRAPH_MAX_VALUES[i] = 1000000.f / FRAME_TIME_GRAPH_MAX_FPS[i]; }

        //scrolling data and average FPS computing
        const std::vector<TimeStamp>& timeStamps = m_pRenderer->GetTimingValues();
        const bool bTimeStampsAvailable = timeStamps.size() > 0;
        if (bTimeStampsAvailable)
        {
            RECENT_HIGHEST_FRAME_TIME = 0;
            FRAME_TIME_ARRAY[NUM_FRAMES - 1] = timeStamps.back().m_microseconds;
            for (uint32_t i = 0; i < NUM_FRAMES - 1; i++)
            {
                FRAME_TIME_ARRAY[i] = FRAME_TIME_ARRAY[i + 1];
            }
            RECENT_HIGHEST_FRAME_TIME = max(RECENT_HIGHEST_FRAME_TIME, FRAME_TIME_ARRAY[NUM_FRAMES - 1]);
        }
        const float& frameTime_us = FRAME_TIME_ARRAY[NUM_FRAMES - 1];
        const float  frameTime_ms = frameTime_us * 0.001f;
        const int fps = bTimeStampsAvailable ? static_cast<int>(1000000.0f / frameTime_us) : 0;

        // UI
        ImGui::SetNextWindowPos(ImVec2((float)PROFILER_WINDOW_POS_X, (float)PROFILER_WINDOW_POS_Y), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(PROFILER_WINDOW_SIZE_X, PROFILER_WINDOW_SIZE_Y), ImGuiCond_FirstUseEver);
        ImGui::Begin("PROFILER (F2)", &m_UIState.bShowProfilerWindow);

        ImGui::Text("Resolution : %ix%i", m_Width, m_Height);
        ImGui::Text("API        : %s", m_systemInfo.mGfxAPI.c_str());
        ImGui::Text("GPU        : %s", m_systemInfo.mGPUName.c_str());
        ImGui::Text("CPU        : %s", m_systemInfo.mCPUName.c_str());
        ImGui::Text("FPS        : %d (%.2f ms)", fps, frameTime_ms);

        if (ImGui::CollapsingHeader("GPU Timings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            std::string msOrUsButtonText = m_UIState.bShowMilliseconds ? "Switch to microseconds" : "Switch to milliseconds";
            if (ImGui::Button(msOrUsButtonText.c_str())) {
                m_UIState.bShowMilliseconds = !m_UIState.bShowMilliseconds;
            }
            ImGui::Spacing();

            if (m_isCpuValidationLayerEnabled || m_isGpuValidationLayerEnabled)
            {
                ImGui::TextColored(ImVec4(1,1,0,1), "WARNING: Validation layer is switched on"); 
                ImGui::Text("Performance numbers may be inaccurate!");
            }

            // find the index of the FrameTimeGraphMaxValue as the next higher-than-recent-highest-frame-time in the pre-determined value list
            size_t iFrameTimeGraphMaxValue = 0;
            for (int i = 0; i < _countof(FRAME_TIME_GRAPH_MAX_VALUES); ++i)
            {
                if (RECENT_HIGHEST_FRAME_TIME < FRAME_TIME_GRAPH_MAX_VALUES[i]) // FRAME_TIME_GRAPH_MAX_VALUES are in increasing order
                {
                    iFrameTimeGraphMaxValue = min(_countof(FRAME_TIME_GRAPH_MAX_VALUES) - 1, i + 1);
                    break;
                }
            }
            ImGui::PlotLines("", FRAME_TIME_ARRAY, NUM_FRAMES, 0, "GPU frame time (us)", 0.0f, FRAME_TIME_GRAPH_MAX_VALUES[iFrameTimeGraphMaxValue], ImVec2(0, 80));

            for (uint32_t i = 0; i < timeStamps.size(); i++)
            {
                float value = m_UIState.bShowMilliseconds ? timeStamps[i].m_microseconds / 1000.0f : timeStamps[i].m_microseconds;
                const char* pStrUnit = m_UIState.bShowMilliseconds ? "ms" : "us";
                ImGui::Text("%-18s: %7.2f %s", timeStamps[i].m_label.c_str(), value, pStrUnit);
            }
        }

        ImGui::Spacing();
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Scene Loading Timings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const auto sceneLoadingTimings = m_pRenderer->GetSceneLoadingTimings();

            for (const auto& sceneTimePtr : sceneLoadingTimings)
            {
                const auto& sceneTime = *sceneTimePtr;
                ImGui::Text("Scene Name: %s", sceneTime.loadRequest.m_sceneName->c_str());
                ImGui::Text("CPU Load Time: %7.2f ms", sceneTime.loadTime);
                if (sceneTime.ioTime > 0) ImGui::Text("IoTime: %7.2f ms", sceneTime.ioTime);
                ImGui::Text("DirectStorage: %s", sceneTime.loadRequest.m_useDirectStorage ? "On" : "Off");
                ImGui::Text("Placed Resources: %s", sceneTime.loadRequest.m_usePlacedResources ? "On" : "Off");
                ImGui::Text("Texture File Size (MiB): %7.2f", sceneTime.sceneTextureFileDataSize/1024.0/1024.0);

                if (sceneTime.ioTime > 0)
                {
                    ImGui::Text("Avg Disk Only Data Rate (MiB/s): %7.2f", sceneTime.sceneTextureFileDataSize / 1024.0 / sceneTime.ioTime);
  
                    if (sceneTime.sceneTextureUncompressedSize != sceneTime.sceneTextureFileDataSize)
                    {
                        ImGui::Text("Avg Amplified Data Rate (MiB/s): %7.2f", sceneTime.sceneTextureUncompressedSize / 1024.0 / sceneTime.ioTime);
                    }
                }

                ImGui::Spacing();               
                ImGui::Spacing();
            }

        }
       
        ImGui::End(); // PROFILER
    }

    {
        // Legend
        ImGui::SetNextWindowPos(ImVec2((float)LEGEND_WINDOW_POS_X, (float)LEGEND_WINDOW_POS_Y), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(LEGEND_WINDOW_SIZE_X, LEGEND_WINDOW_SIZE_Y), ImGuiCond_FirstUseEver);
        if( ImGui::Begin("LEGEND") )
        {
            ImGui::ColorButton("", ImVec4(0, 0, 0, 0.0), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs, ImVec2(60, 60));
            ImGui::SameLine(); ImGui::Text("Unloaded");
            ImGui::ColorButton("", ImVec4(0, 1, 0, 1), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs, ImVec2(60, 60));
            ImGui::SameLine(); ImGui::Text("Loaded");
            ImGui::ColorButton("", ImVec4(1, 1, 0, 1), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs, ImVec2(60, 60));
            ImGui::SameLine(); ImGui::Text("Loading");
            ImGui::ColorButton("", ImVec4(1, 0, 0, 1), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs, ImVec2(60, 60));
            ImGui::SameLine(); ImGui::Text("Cancelling");
            ImGui::ColorButton("", ImVec4(1, 0.423, 0, 1), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs, ImVec2(60, 60));
            ImGui::SameLine(); ImGui::Text("Unloading");

        }
        ImGui::End();
    }
 }

void UIState::Initialize()
{
    // init GUI state
    this->SelectedTonemapperIndex = 0;
    this->bUseTAA = true;
    this->SelectedSkydomeTypeIndex = 0;
    this->Exposure = 1.0f;
    this->IBLFactor = 2.0f;
    this->EmissiveFactor = 1.0f;
    this->bDrawLightFrustum = false;
    this->bDrawBoundingBoxes = false;
    this->WireframeMode = WireframeMode::WIREFRAME_MODE_OFF;
    this->WireframeColor[0] = 0.0f;
    this->WireframeColor[1] = 1.0f;
    this->WireframeColor[2] = 0.0f;
    this->bShowControlsWindow = true;
    this->bShowProfilerWindow = true;
    this->bAutopilot = true;
}

