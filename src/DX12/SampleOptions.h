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

struct ProfilerOptions
{
    bool m_bProfilerOutputEnabled = false;
    std::string m_profilerOutputPath{ "profilerOutput.csv" };
    float m_profileSeconds = 0;
    bool m_bioTiming = false;
};

struct IOOptions
{
    bool m_useDirectStorage = false;

    bool m_usePlacedResources = false;
    uint32_t m_stagingBufferSize = 192 * 1024 * 1024;
    uint32_t m_queueLength = 128;

    bool m_allowCancellation = false;
    bool m_disableGPUDecompression = false;
    bool m_disableMetaCommand = false;
};

struct CameraOptions
{
    bool m_cameraVelocityNonLinear = false;
    float m_cameraSpeed = 1.0f;
};

struct DisplayOptions
{
    float m_fontSize = 13.0f;
};

struct WorkloadOptions
{
    uint32_t m_mandelbrotIterations = 0;
};


struct SampleOptions
{
    DisplayOptions displayOptions;
    CameraOptions cameraOptions;
    IOOptions ioOptions;
    ProfilerOptions profilerOptions;
    WorkloadOptions workloadOptions;
};
