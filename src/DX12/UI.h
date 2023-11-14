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

struct UIState
{
    //
    // WINDOW MANAGEMENT
    //
    bool bShowControlsWindow;
    bool bShowProfilerWindow;

    //
    // POST PROCESS CONTROLS
    //
    int   SelectedTonemapperIndex;
    float Exposure;

    bool  bUseTAA;

    //
    // APP/SCENE CONTROLS
    //
    float IBLFactor;
    float EmissiveFactor;

    int   SelectedSkydomeTypeIndex;
    bool  bDrawBoundingBoxes;
    bool  bDrawLightFrustum;

    enum class WireframeMode : int
    {
        WIREFRAME_MODE_OFF = 0,
        WIREFRAME_MODE_SHADED = 1,
        WIREFRAME_MODE_SOLID_COLOR = 2,
    };

    WireframeMode WireframeMode;
    float         WireframeColor[3];

    //
    // PROFILER CONTROLS
    //
    bool  bShowMilliseconds;


    //
    // INPUT CONTROL
    //
    bool bAutopilot;

    // -----------------------------------------------

    void Initialize();
};