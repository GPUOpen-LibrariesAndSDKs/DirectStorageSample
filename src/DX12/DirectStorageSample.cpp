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
#include <intrin.h>
#include "DirectStorageSample.h"
#include "DirectStorageSampleTexturePackageFormat.h"
#include "GLTFTextureAndBuffersDirectStorage.h"
#include "Misc/CPUUserMarkers.h"
#include "Profile.h"


DirectStorageSample::DirectStorageSample(LPCSTR name) : FrameworkWindows(name)
{
    m_time = 0;
    m_bPlay = true;
}

//--------------------------------------------------------------------------------------
//
// OnParseCommandLine
//
//--------------------------------------------------------------------------------------
void DirectStorageSample::OnParseCommandLine(LPSTR lpCmdLine, uint32_t* pWidth, uint32_t* pHeight)
{
    // set some default values
    *pWidth = 1920; 
    *pHeight = 1080; 
    m_VsyncEnabled = false;
    m_isCpuValidationLayerEnabled = false;
    m_isGpuValidationLayerEnabled = false;
    m_stablePowerState = false;

    //read globals
    auto process = [&](json jData)
    {
        // framework options
        *pWidth = jData.value("width", *pWidth);
        *pHeight = jData.value("height", *pHeight);
        m_fullscreenMode = jData.value("presentationMode", m_fullscreenMode);
        m_stablePowerState = jData.value("stablePowerState", m_stablePowerState);

        m_initializeAGS = jData.value("initializeAgs", m_initializeAGS);
        m_isCpuValidationLayerEnabled = jData.value("CpuValidationLayerEnabled", m_isCpuValidationLayerEnabled);
        m_isGpuValidationLayerEnabled = jData.value("GpuValidationLayerEnabled", m_isGpuValidationLayerEnabled);
        m_VsyncEnabled = jData.value("vsync", m_VsyncEnabled);
        m_FreesyncHDROptionEnabled = jData.value("FreesyncHDROptionEnabled", m_FreesyncHDROptionEnabled);

        
        // Sample specific options

        // profiler options
        m_sampleOptions.profilerOptions.m_bProfilerOutputEnabled = jData.value("profile", m_sampleOptions.profilerOptions.m_bProfilerOutputEnabled);
        m_sampleOptions.profilerOptions.m_profilerOutputPath = jData.value("profileOutputPath", m_sampleOptions.profilerOptions.m_profilerOutputPath);
        m_sampleOptions.profilerOptions.m_profileSeconds = jData.value("profileseconds", m_sampleOptions.profilerOptions.m_profileSeconds);
        m_sampleOptions.profilerOptions.m_bioTiming = jData.value("iotiming", m_sampleOptions.profilerOptions.m_bioTiming);

        // IO options.
        m_sampleOptions.ioOptions.m_useDirectStorage = jData.value("directstorage", m_sampleOptions.ioOptions.m_useDirectStorage);
        m_sampleOptions.ioOptions.m_usePlacedResources = jData.value("placedresources", m_sampleOptions.ioOptions.m_usePlacedResources);
        m_sampleOptions.ioOptions.m_stagingBufferSize = jData.value("stagingbuffersize", m_sampleOptions.ioOptions.m_stagingBufferSize);
        m_sampleOptions.ioOptions.m_queueLength = jData.value("queuelength", m_sampleOptions.ioOptions.m_queueLength);
        m_sampleOptions.ioOptions.m_allowCancellation = jData.value("allowcancellation", m_sampleOptions.ioOptions.m_allowCancellation);
        m_sampleOptions.ioOptions.m_disableGPUDecompression = jData.value("disablegpudecompression", m_sampleOptions.ioOptions.m_disableGPUDecompression);
        m_sampleOptions.ioOptions.m_disableMetaCommand = jData.value("disablemetacommand", m_sampleOptions.ioOptions.m_disableMetaCommand);

        // camera options
        m_sampleOptions.cameraOptions.m_cameraSpeed = jData.value("cameraspeed", m_sampleOptions.cameraOptions.m_cameraSpeed);
        m_sampleOptions.cameraOptions.m_cameraVelocityNonLinear = jData.value("cameravelocitynonlinear", m_sampleOptions.cameraOptions.m_cameraVelocityNonLinear);

        // display options
        m_sampleOptions.displayOptions.m_fontSize = jData.value("fontsize", m_sampleOptions.displayOptions.m_fontSize);

        // workload options
        m_sampleOptions.workloadOptions.m_mandelbrotIterations = jData.value("mandelbrotiterations", m_sampleOptions.workloadOptions.m_mandelbrotIterations);
    };

    //read json globals from commandline
    //
    try
    {
        if (strlen(lpCmdLine) > 0)
        {
            auto j3 = json::parse(lpCmdLine);
            process(j3);
        }
    }
    catch (json::parse_error)
    {
        Trace("Error parsing commandline\n");
        exit(0);
    }

    // read config file (and override values from commandline if so)
    //
    {
        std::ifstream f("DirectStorageSample.json");
        if (!f)
        {
            MessageBox(NULL, "Config file not found!\n", "Cauldron Panic!", MB_ICONERROR);
            exit(0);
        }

        try
        {
            f >> m_jsonConfigFile;
        }
        catch (json::parse_error)
        {
            MessageBox(NULL, "Error parsing DirectStorageSample.json!\n", "Cauldron Panic!", MB_ICONERROR);
            exit(0);
        }
    }


    json globals = m_jsonConfigFile["globals"];
    process(globals);

    json scenes = m_jsonConfigFile["scenes"];

    // get the list of scenes
    for (const auto & scene : scenes)
        m_sceneNames.push_back(scene["name"]);

    
    // parse streaming volumes.
    const auto& jsonStreamingVolumes = m_jsonConfigFile["StreamingVolumes"];
    const auto streamingVolumeCount = jsonStreamingVolumes.size();
    m_StreamingVolumes.resize(streamingVolumeCount);
    for (size_t volIdx = 0; volIdx < streamingVolumeCount; volIdx++)
    {
        StreamingVolume& volume = m_StreamingVolumes[volIdx];
        json::object_t volJsonNode = jsonStreamingVolumes[volIdx];
        volume.m_center = math::Point3(GetElementVector(volJsonNode, "position", math::Vector4(0, 0, 0, 0)).getXYZ());
        volume.m_radius = math::Vector3(GetElementVector(volJsonNode, "radius", math::Vector4(0.5, 0.5, 0.5, 0)).getXYZ());

        auto sceneTransformNodeItr = volJsonNode.find("sceneTransform");
        if (sceneTransformNodeItr != volJsonNode.end())
        {
            auto sceneTransformArrayNode = sceneTransformNodeItr->second.get<json::array_t>();
            volume.m_streamedSceneDataTransform = GetMatrix(sceneTransformArrayNode);

        }
        else
        {
            volume.m_streamedSceneDataTransform = math::Matrix4::identity();
        }

        volume.m_sceneName = GetElementString(volJsonNode, "sceneName", "Unknown");
    }

    // create scene name to path pair map.
    for (const auto& scene : scenes)
    {
        m_sceneNameToScenePath.insert_or_assign(scene["name"], ScenePathPair{ scene["directory"], scene["filename"] });
    }
      

}

//--------------------------------------------------------------------------------------
//
// OnCreate
//
//--------------------------------------------------------------------------------------
void DirectStorageSample::OnCreate()
{
    //init the shader compiler
    InitDirectXCompiler();
    CreateShaderCache();

    if (m_sampleOptions.ioOptions.m_useDirectStorage)
    {
        Sample::InitializeDirectStorage(m_device.GetDevice(), L"..\\media\\", m_sceneNameToScenePath, m_sampleOptions.ioOptions);
    }
    

    // Create a instance of the renderer and initialize it, we need to do that for each GPU
    m_pRenderer = new Renderer();
    m_pRenderer->OnCreate(&m_device, &m_swapChain, &m_AsyncPool, m_sampleOptions.displayOptions.m_fontSize, &m_sceneNameToScenePath, &m_StreamingVolumes, &m_sampleOptions);

    // init GUI (non gfx stuff)
    ImGUI_Init((void *)m_windowHwnd);
    m_UIState.Initialize();

    OnResize(true);
    OnUpdateDisplay();

    // Init Camera, looking at the origin
    m_camera.LookAt(math::Vector4(0, 0, 5, 0), math::Vector4(0, 0, 0, 0));    
}

//--------------------------------------------------------------------------------------
//
// OnDestroy
//
//--------------------------------------------------------------------------------------
void DirectStorageSample::OnDestroy()
{
    // Save what's available and shutdown. Don't want to include data during shutdown.


    if (m_sampleOptions.profilerOptions.m_bProfilerOutputEnabled)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utf8utf16converter;
        m_profiler.Save(utf8utf16converter.from_bytes(m_sampleOptions.profilerOptions.m_profilerOutputPath).c_str());
    }

    m_AsyncPool.Flush();
    ImGUI_Shutdown();
    ShutdownStreaming();

    m_device.GPUFlush();
 


    m_pRenderer->OnDestroyWindowSizeDependentResources();
    m_pRenderer->OnDestroy();

    delete m_pRenderer;

    if (m_sampleOptions.ioOptions.m_useDirectStorage)
    {
        // shutdown DirectStorage.
        Sample::ShutdownDirectStorage();
    }

    //shut down the shader compiler 
    DestroyShaderCache(&m_device);
}

//--------------------------------------------------------------------------------------
//
// OnEvent, win32 sends us events and we forward them to ImGUI
//
//--------------------------------------------------------------------------------------
bool DirectStorageSample::OnEvent(MSG msg)
{
    if (ImGUI_WndProcHandler(msg.hwnd, msg.message, msg.wParam, msg.lParam))
        return true;

    // handle function keys (F1, F2...) here, rest of the input is handled
    // by imGUI later in HandleInput() function
    const WPARAM& KeyPressed = msg.wParam;
    switch (msg.message)
    {
    case WM_KEYUP:
    case WM_SYSKEYUP:
        /* WINDOW TOGGLES */
        if (KeyPressed == VK_F1) m_UIState.bShowControlsWindow ^= 1;
        if (KeyPressed == VK_F2) m_UIState.bShowProfilerWindow ^= 1;
        break;
    }

    return true;
}

//--------------------------------------------------------------------------------------
//
// OnResize
//
//--------------------------------------------------------------------------------------
void DirectStorageSample::OnResize(bool resizeRender)
{
	// Destroy resources (if we are not minimized)
    if (resizeRender && m_Width && m_Height && m_pRenderer)
    {
        m_pRenderer->OnDestroyWindowSizeDependentResources();
        m_pRenderer->OnCreateWindowSizeDependentResources(&m_swapChain, m_Width, m_Height);
    }

    m_camera.SetFov(AMD_PI_OVER_4, m_Width, m_Height, 0.1f, 1000.0f);
}

//--------------------------------------------------------------------------------------
//
// UpdateDisplay
//
//--------------------------------------------------------------------------------------
void DirectStorageSample::OnUpdateDisplay()
{
    // Destroy resources (if we are not minimized)
    if (m_pRenderer)
    {
        m_pRenderer->OnUpdateDisplayDependentResources(&m_swapChain);
    }
}

void DirectStorageSample::CheckAndRequestScenesToLoad()
{
    CPUUserMarker marker("CheckAndRequestScenesToLoad");
    // Get eye pos.
    const auto eyePos = math::Point3(m_camera.GetPosition().get128());

    auto& streamingVolumes(m_StreamingVolumes);
    for (auto& vol : streamingVolumes)
    {
        if (vol.m_LoadingState == GLTFLoadingState::Unloaded && vol.IsPointInside(eyePos))
        {
            vol.m_LoadingState = GLTFLoadingState::Loading;

            SceneLoadRequest loadRequest;
            loadRequest.workloadId = (m_sampleOptions.ioOptions.m_useDirectStorage && m_sampleOptions.profilerOptions.m_bioTiming) ? Sample::DStorageBeginProfileLoading() : 0;
            loadRequest.m_sceneName = &vol.m_sceneName;
            loadRequest.m_streamedSceneDataTransform = vol.m_streamedSceneDataTransform;
            loadRequest.m_useDirectStorage = m_sampleOptions.ioOptions.m_useDirectStorage;
            loadRequest.m_usePlacedResources = m_sampleOptions.ioOptions.m_usePlacedResources;

            vol.workloadId = loadRequest.workloadId;
            vol.m_LoadedSceneFuture = std::move(m_pRenderer->LoadSceneAsync(loadRequest));
        }

        if (m_sampleOptions.ioOptions.m_allowCancellation && vol.m_LoadingState == GLTFLoadingState::Loading && !vol.IsPointInside(eyePos))
        {
            // Cancel scene async
            vol.m_LoadingState = GLTFLoadingState::CancellationRequested;
            if (m_sampleOptions.ioOptions.m_useDirectStorage) Sample::DStorageCancelRequest(vol.workloadId);
        }

        if (vol.m_LoadingState == GLTFLoadingState::CancellationRequested)
        {
            auto status = vol.m_LoadedSceneFuture.wait_for(std::chrono::duration<int>(0));
            if (status == std::future_status::ready)
            {
                assert(vol.m_pSceneData == nullptr);
                vol.m_pSceneData = vol.m_LoadedSceneFuture.get();
                vol.m_pSceneData->m_timingData.frameTimes.Reset();
                vol.m_LoadingState = GLTFLoadingState::Unloading;
                vol.m_LoadedSceneFuture = std::move(m_pRenderer->UnloadSceneAsync(vol.m_sceneName, vol.m_pSceneData));
            }
        }

        if (vol.m_LoadingState == GLTFLoadingState::Loading)
        {            
            auto status = vol.m_LoadedSceneFuture.wait_for(std::chrono::duration<int>(0));
            if (status == std::future_status::ready)
            {
                assert(vol.m_pSceneData == nullptr);
                vol.m_pSceneData = vol.m_LoadedSceneFuture.get();
                vol.m_LoadingState = GLTFLoadingState::Loaded;

                if (m_sampleOptions.ioOptions.m_useDirectStorage && m_sampleOptions.profilerOptions.m_bioTiming) vol.m_pSceneData->m_timingData.ioTime = Sample::DStorageProfileRetrieveTiming(vol.workloadId);

                if (m_sampleOptions.profilerOptions.m_bProfilerOutputEnabled)
                {
                    vol.m_pSceneData->m_timingData.frameTimes = std::move(vol.frameTimes);
                    m_profiler.AddData(vol.m_pSceneData->m_timingData);
                }

                // Notify scene loaded.
                m_pRenderer->AddScene(vol.m_pSceneData);
            }
            else
            {
                vol.frameTimes.AddTime(m_pRenderer->GetLastFrameTiming());
            }
        }

        if (vol.m_LoadingState == GLTFLoadingState::Loaded && !vol.IsPointInside(eyePos))
        {
            vol.m_LoadingState = GLTFLoadingState::Unloading;
            vol.m_pSceneData->m_timingData.frameTimes.Reset();
            m_pRenderer->RemoveScene(vol.m_pSceneData);
            vol.m_LoadedSceneFuture = std::move(m_pRenderer->UnloadSceneAsync(vol.m_sceneName, vol.m_pSceneData));
        }

        if (vol.m_LoadingState == GLTFLoadingState::Unloading)
        {
            auto status = vol.m_LoadedSceneFuture.wait_for(std::chrono::duration<int>(0));
            if (status == std::future_status::ready)
            {
                vol.m_pSceneData = vol.m_LoadedSceneFuture.get(); // should be null ptr.
                vol.m_LoadingState = GLTFLoadingState::Unloaded;
            }
        }
    }
}

void DirectStorageSample::ShutdownStreaming()
{
    {
        // wait for all in-flight operations to end.
        auto waitForInFlightOperations = [this]() {
            bool isInFlight;
            do
            {
                isInFlight = false;

                auto& streamingVolumes(m_StreamingVolumes);
                for (auto& vol : streamingVolumes)
                {

                    if (vol.m_LoadingState == GLTFLoadingState::Loading)
                    {
                        auto status = vol.m_LoadedSceneFuture.wait_for(std::chrono::duration<int>(0));
                        if (status == std::future_status::ready)
                        {
                            vol.m_pSceneData = vol.m_LoadedSceneFuture.get();
                            vol.m_LoadingState = GLTFLoadingState::Loaded;
                        }

                        isInFlight = true;
                    }

                    if (vol.m_LoadingState == GLTFLoadingState::Unloading)
                    {
                        auto status = vol.m_LoadedSceneFuture.wait_for(std::chrono::duration<int>(0));
                        if (status == std::future_status::ready)
                        {
                            vol.m_pSceneData = vol.m_LoadedSceneFuture.get(); // should be null ptr.
                            vol.m_LoadingState = GLTFLoadingState::Unloaded;
                            isInFlight = true;
                        }

                        isInFlight = true;
                    }
                }
            } while (isInFlight);
        };

        waitForInFlightOperations();

        // Request scene unload.
        for (auto& vol : m_StreamingVolumes)
        {
            if (vol.m_LoadingState == GLTFLoadingState::Loaded)
            {
                vol.m_LoadedSceneFuture = m_pRenderer->UnloadSceneAsync(vol.m_sceneName,vol.m_pSceneData);
            }
        }

        waitForInFlightOperations();
    }
}


//--------------------------------------------------------------------------------------
//
// OnUpdate
//
//--------------------------------------------------------------------------------------
void DirectStorageSample::OnUpdate()
{
    ImGuiIO& io = ImGui::GetIO();

    //If the mouse was not used by the GUI then it's for the camera
    if (io.WantCaptureMouse)
    {
        io.MouseDelta.x = 0;
        io.MouseDelta.y = 0;
        io.MouseWheel = 0;
    }



    // Update Camera
    UpdateCamera(m_camera, &io);
    if (m_UIState.bUseTAA)
    {
        static uint32_t Seed;
        m_camera.SetProjectionJitter(m_Width, m_Height, Seed);
    }
    else
        m_camera.SetProjectionJitter(0.f, 0.f);

    // Keyboard & Mouse
    HandleInput(io);

    // Animation Update
    if (m_bPlay)
        m_time += (float)m_deltaTime / 1000.0f; // animation time in seconds

    static double timeRunning = MillisecondsNow();
    if ((m_sampleOptions.profilerOptions.m_profileSeconds > 0.0f) && ((MillisecondsNow() - timeRunning)/1000) > m_sampleOptions.profilerOptions.m_profileSeconds)
    {
        PostQuitMessage(0);
    }
}
void DirectStorageSample::HandleInput(const ImGuiIO& io)
{
}

void DirectStorageSample::UpdateCamera(Camera& cam, const ImGuiIO* io)
{
    cam.UpdatePreviousMatrices(); // set previous view matrix for TAA and other algorithms.

    if (!io)
        return;
    static Vectormath::SSE::Point3 lastPosition = Vectormath::SSE::Point3(0,0,0);

    if (m_UIState.bAutopilot)
    {
        static int sourceIdx = 0;
        static int targetIdx = 1;
        static int directionalOffset = 1;
        auto startPos = m_StreamingVolumes[sourceIdx].m_center;
        auto endPos = m_StreamingVolumes[targetIdx].m_center;

        // divide time down otherwise it's too fast (alternative is scale speed).
        double time = m_deltaTime / 1000.0;

        // get direction.
        auto direction = Vectormath::SSE::normalize(endPos - startPos);


        double speed = 1.0;

        if (m_sampleOptions.cameraOptions.m_cameraVelocityNonLinear)
        {
            // set camera speed
            double distanceToTarget = Vectormath::SSE::length(endPos - lastPosition);
            double distanceTotal = Vectormath::SSE::length(endPos - startPos);
            double travelProgressRatio = distanceToTarget / distanceTotal;

            speed = (-(cos(3.1415 * travelProgressRatio) - 1.0) / 2.0) + 0.2;
        }

        // Sf = Vt + So.
        auto currentPosition = direction * speed * time + lastPosition;

        // clamp position to inside bounds of the points.
        auto maxs = Vectormath::SSE::maxPerElem(startPos, endPos);
        auto mins = Vectormath::SSE::minPerElem(startPos, endPos);
        auto clampedPosition = Vectormath::SSE::maxPerElem(Vectormath::SSE::minPerElem(maxs, currentPosition), mins);

        // If the last position is the same as the current clamped position, we are at the target position and we should change targets.
        int mask = _mm_movemask_ps(_mm_cmpeq_ps(lastPosition.get128(), clampedPosition.get128()));
        if (mask == 0xF)
        {
            // now change targets.
            sourceIdx = (sourceIdx + directionalOffset) % m_StreamingVolumes.size();
            targetIdx = (sourceIdx + directionalOffset) % m_StreamingVolumes.size();

            // change directions if necessary and set the next target.
            if (abs(sourceIdx - targetIdx) >= 1)
            {
                directionalOffset = targetIdx < sourceIdx ? -1 : 1;
                targetIdx = sourceIdx + directionalOffset;
            }
        }

        // record last position.
        lastPosition = clampedPosition;

        // set camera
        auto translateMat = math::Matrix4::translation(math::Vector3(0, 0, -1));
        cam.LookAt(math::Vector4(clampedPosition), translateMat * math::Vector4(clampedPosition));
        return;
    }

    float yaw = cam.GetYaw();
    float pitch = cam.GetPitch();

    // Sets Camera based on UI selection (WASD, Orbit or any of the GLTF cameras)
    if ((io->KeyCtrl == false) && (io->MouseDown[0] == true))
    {
        yaw -= io->MouseDelta.x / 100.f;
        pitch += io->MouseDelta.y / 100.f;
    }

    //  WASD
   cam.UpdateCameraWASD(yaw, pitch, io->KeysDown, m_deltaTime/1000.0);
   lastPosition = Vectormath::SSE::Point3(cam.GetPosition().get128());
}

//--------------------------------------------------------------------------------------
//
// OnRender
//
//--------------------------------------------------------------------------------------
void DirectStorageSample::OnRender()
{
    // Do any start of frame necessities
    BeginFrame();

    m_deltaTime *= m_sampleOptions.cameraOptions.m_cameraSpeed;

    ImGUI_UpdateIO();
    ImGui::NewFrame();

    BuildUI(); // UI logic. Note that the rendering of the UI happens later.

    OnUpdate();
    CheckAndRequestScenesToLoad();

    TimeState timeState = {};
    timeState.m_deltaTime = m_deltaTime;
    timeState.m_animationTime = m_time;

    // Do Render frame using AFR
    m_pRenderer->OnRender(&m_UIState, m_camera, timeState, &m_swapChain);

    // Framework will handle Present and some other end of frame logic
    EndFrame();
}


//--------------------------------------------------------------------------------------
//
// WinMain
//
//--------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{

    SetThreadName(GetCurrentThread(), "Main/Rendering");
    //auto flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    //_CrtSetDbgFlag(flags | _CRTDBG_CHECK_ALWAYS_DF);

    LPCSTR Name = "DirectStorage Sample";

    // create new DX sample
    return RunFramework(hInstance, lpCmdLine, nCmdShow, new DirectStorageSample(Name));
}
