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
#include "Misc/Misc.h"
#include "GLTF/GltfHelpers.h"
#include "Base/ShaderCompiler.h"
#include "Misc/ThreadPool.h"
#include "Renderer.h"
#include "GLTFTextureAndBuffersDirectStorage.h"
#include "DirectStorageSampleTexturePackageFormat.h"
#include "../common/GLTF/GltfPbrMaterial.h"
#include <stack>
#include <dstorage.h>
#include <codecvt>
#include <unordered_map>
#include "misc/DxgiFormatHelper.h"
#include "PackageUtils.h"
#include "Misc/CPUUserMarkers.h"
#include "DirectStorageSample.h"


template<>
struct std::hash<ScenePathPair>
{
    std::size_t operator()(const ScenePathPair& sp) const noexcept
    {
        return std::hash<std::string>().operator()(sp.sceneFile) ^ std::hash<std::string>().operator()(sp.scenePath);
    }
};

template<>
struct std::equal_to<ScenePathPair>
{
    bool operator()(const ScenePathPair& lhs, const ScenePathPair& rhs) const
    {
        return  lhs.sceneFile == rhs.sceneFile && lhs.scenePath == rhs.scenePath;
    }
};

namespace Sample
{
    struct ResourceLookupEntry
    {
        const DirectStorageSampleTextureMetadataHeader* metaDataHeader = nullptr;
        uint64_t resourceHeapOffset = 0;
        uint64_t resourceHeapSize = 0;
        IDStorageFile* reseourceFileHandle = nullptr;
        std::string gltfPath; // really debug data.
    };

    static IDStorageFactory* g_DStorageFactory = nullptr;
    static IDStorageQueue* g_DStorageQueueNormal = nullptr;
    static IDStorageQueue* g_DStorageQueueRealtime = nullptr;
    static ID3D12Fence* g_DStorageFenceGPU = nullptr;
    static std::atomic<UINT64> g_DStorageFenceValueGPU = 0;
    static ID3D12Fence* g_DStorageFenceCPU = nullptr;
    static HANDLE g_DStorageFenceCPUEvent = INVALID_HANDLE_VALUE;
    static std::atomic<UINT64> g_DStorageFenceValueCPU = 0;
    static ID3D12Fence* g_DStorageFenceProfile = nullptr;
    static HANDLE g_DStorageFenceProfileEvent = INVALID_HANDLE_VALUE;
    static std::atomic<UINT64> g_DStorageFenceValueProfile = 0;
    static std::vector<std::vector<DirectStorageSampleTextureMetadataHeader>> g_MetaDataHeaders;
    static std::unordered_map<std::wstring, ResourceLookupEntry> g_ResourceTable;
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> g_Converter;
    static const std::unordered_map<std::string, ScenePathPair>* g_pScenePathMap = nullptr;  
    static std::unordered_map<ScenePathPair, D3D12_HEAP_DESC> g_SceneHeapTemplates;
    static std::unordered_map<ScenePathPair, size_t> g_SceneTextureDataSizeOnDisk;
    static std::unordered_map<ScenePathPair, size_t> g_SceneTextureDataSizeUncompressed;
    static std::vector<IDStorageFile*> g_FileHandles;

    D3D12_HEAP_DESC GetTextureHeapDescForScene(const ScenePathPair& scenePathPair)
    {
        return g_SceneHeapTemplates[scenePathPair];
    }
   
    size_t GetSceneTextureDataSizeUncompressed(const ScenePathPair& scenePathPair)
    {
        return g_SceneTextureDataSizeUncompressed.at(scenePathPair);
    }

    size_t GetSceneTextureDataSizeOnDisk(const ScenePathPair& scenePathPair)
    {
        return g_SceneTextureDataSizeOnDisk.at(scenePathPair);
    }

    double GetSceneTextureCompressionRatio(const ScenePathPair& scenePathPair)
    {
        return g_SceneTextureDataSizeUncompressed.at(scenePathPair) / (double)g_SceneTextureDataSizeOnDisk.at(scenePathPair);
    }


    bool Texture::InitFromFile(Device* pDevice, UploadHeap* pUploadHeap, ID3D12Heap* pTextureHeap, const char* szFilename, uint64_t workloadId, bool useSRGB, float cutOff, D3D12_RESOURCE_FLAGS resourceFlags)
    {
        // Get Desc from file.
        auto fileName = g_Converter.from_bytes(szFilename);     

        const auto& resourceEntry = g_ResourceTable[fileName];
        const auto& metaDataHeader = resourceEntry.metaDataHeader;

        CD3DX12_RESOURCE_DESC RDescs(metaDataHeader->resourceDesc);
        RDescs.Format = SetFormatGamma((DXGI_FORMAT)RDescs.Format, useSRGB);
 
        // If the caller passed in a heap, attempt to use placed resources.
        if (pTextureHeap)
        {
            HRESULT hr = pDevice->GetDevice()->CreatePlacedResource(pTextureHeap
                , resourceEntry.resourceHeapOffset
                , &RDescs
                , D3D12_RESOURCE_STATE_COMMON
                , nullptr
                , IID_PPV_ARGS(&m_pResource));
            assert(hr == S_OK);
        }
        else
        {

            HRESULT hr = pDevice->GetDevice()->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_CREATE_NOT_ZEROED,
                &RDescs,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&m_pResource));

            assert(hr == S_OK);
        }


        // stuff needed later that's not important here. We are using this to create the SRV. It's an adapter for some reason.
        m_header.format = RDescs.Format;
        m_header.bitCount = BitsPerPixel(RDescs.Format);
        m_header.mipMapCount = RDescs.MipLevels;
        m_header.arraySize = RDescs.ArraySize();
        m_header.depth = RDescs.Depth();
        m_header.width = RDescs.Width;
        m_header.height = RDescs.Height;

        const auto& fileHandle = resourceEntry.reseourceFileHandle;

        assert((metaDataHeader->resourceOffset % 4096) == 0);

        // perform the read.
        DSTORAGE_REQUEST req = {};
        req.Options.CompressionFormat = metaDataHeader->compressionFormat;
        req.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
        req.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MULTIPLE_SUBRESOURCES;
        req.Source.File.Source = fileHandle;
        req.Source.File.Offset = metaDataHeader->resourceOffset;
        req.Source.File.Size = metaDataHeader->resourceSizeCompressed;
        req.Destination.MultipleSubresources.Resource = m_pResource;
        req.Destination.MultipleSubresources.FirstSubresource = 0;
        req.UncompressedSize = metaDataHeader->resourceSizeUncompressed;


        req.CancellationTag = workloadId;
        req.Name = (char*)resourceEntry.gltfPath.c_str();
        g_DStorageQueueNormal->EnqueueRequest(&req);
        //g_DStorageQueueNormal->Submit();
       
        return true;
    }

#include "GLTF/GLTFTexturesAndBuffersImpl.inl"

    struct DStorageErrorEventHandles
    {
        HANDLE DStorageErrorHandle = INVALID_HANDLE_VALUE;
        HANDLE RegisteredWaitHandle = INVALID_HANDLE_VALUE;
    };

#include <strsafe.h>
    VOID NTAPI DStorageErrorHandler(PVOID pData, BOOLEAN bTimeout)
    {
        wchar_t errorStringBuffer[max(sizeof(DSTORAGE_ERROR_PARAMETERS_REQUEST),4096)];
        
        IDStorageQueue* sourceQueue = reinterpret_cast<IDStorageQueue*>(pData);
        DSTORAGE_ERROR_RECORD errorRecord;
        sourceQueue->RetrieveErrorRecord(&errorRecord);

        if (errorRecord.FailureCount > 0)
        {
            const auto& failureRecord = errorRecord.FirstFailure;
            
            //_com_error comError(failureRecord.HResult);
            wchar_t windowsErrorBuffer[4096];
            memset(windowsErrorBuffer, 0, sizeof windowsErrorBuffer);
            FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, failureRecord.HResult, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), windowsErrorBuffer, std::extent_v<decltype(windowsErrorBuffer)>, NULL);

            switch (failureRecord.CommandType)
            {
            case DSTORAGE_COMMAND_TYPE_NONE:
                // Failure wasn't for a command... interesting.
                break;
               
            case DSTORAGE_COMMAND_TYPE_REQUEST:
                memset(errorStringBuffer, 0, sizeof(errorStringBuffer));
                

                StringCbPrintfW(errorStringBuffer, sizeof(errorStringBuffer)
#ifdef UNICODE
                    , L"HRESULT Error:%ls\n"
#else
                    , L"HRESULT Error:%hs\n"
#endif
                    L"\tCommand: %ls\n"
                    L"\tFilename: %ls\n"
                    L"\tRequestName: %hs\n"
                    , windowsErrorBuffer
                    , L"Request"
                    , failureRecord.Request.Filename
                    , failureRecord.Request.RequestName);                 
                break;

            case DSTORAGE_COMMAND_TYPE_STATUS:
                break;

            case DSTORAGE_COMMAND_TYPE_SIGNAL:
                break;

            default:
                // Unknown command type.
                break;
            }
        }

        assert(!"DirectStorage error.");
        return;
    }


    bool InitializeDirectStorage(ID3D12Device* const pDevice, const std::wstring& contentPathRoot, const std::unordered_map<std::string, ScenePathPair>& scenePathMap, const IOOptions& ioOptions)
    {
        CPUUserMarker marker("InitializeDirectStorage");


        g_pScenePathMap = &scenePathMap;

        {
            // Create DirectStorage loader.
            DSTORAGE_CONFIGURATION dstorageConfig = {};
            dstorageConfig.DisableTelemetry = TRUE;
            dstorageConfig.DisableGpuDecompression = ioOptions.m_disableGPUDecompression;
            dstorageConfig.DisableGpuDecompressionMetacommand = ioOptions.m_disableMetaCommand;
            ThrowIfFailed(DStorageSetConfiguration(&dstorageConfig)); // must call before getting factory.

            // Get DirectStorage factory.
            ThrowIfFailed(DStorageGetFactory(IID_PPV_ARGS(&g_DStorageFactory)));
        }

        g_DStorageFactory->SetDebugFlags(DSTORAGE_DEBUG_NONE);
        g_DStorageFactory->SetStagingBufferSize(ioOptions.m_stagingBufferSize); // This will depend on assets.
        
        {
            // Normal priority, File->GPU.
            DSTORAGE_QUEUE_DESC queueDesc = {};
            queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
            queueDesc.Capacity = ioOptions.m_queueLength;
            queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
            queueDesc.Name = "NormalPriorityFileToGPU";
            queueDesc.Device = pDevice; // Associate with GPU.
            ThrowIfFailed(g_DStorageFactory->CreateQueue(&queueDesc, IID_PPV_ARGS(&g_DStorageQueueNormal)));

            ThrowIfFailed(pDevice->CreateFence(g_DStorageFenceValueGPU, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_DStorageFenceGPU)));
            ThrowIfFailed(pDevice->CreateFence(g_DStorageFenceValueCPU, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_DStorageFenceCPU)));
            ThrowIfFailed(pDevice->CreateFence(g_DStorageFenceValueProfile, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_DStorageFenceProfile)));

            g_DStorageFenceCPUEvent = CreateEvent(nullptr, false, false, nullptr);
            g_DStorageFenceProfileEvent = CreateEvent(nullptr, false, false, nullptr);
        }


        {
            // Real-time priority. File->Memory. For metadata.
            // Queue only used for metadata.
            DSTORAGE_QUEUE_DESC queueDesc = {};
            queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
            queueDesc.Capacity = DSTORAGE_MIN_QUEUE_CAPACITY;
            queueDesc.Priority = DSTORAGE_PRIORITY_REALTIME;
            queueDesc.Name = "RealtimePriorityFileToMemory";

            ThrowIfFailed(g_DStorageFactory->CreateQueue(&queueDesc, IID_PPV_ARGS(&g_DStorageQueueRealtime)));
        }

        // Setup async error handler and thread.
        DStorageErrorEventHandles DStorageQueueRealtimeErrorHandles;
        DStorageQueueRealtimeErrorHandles.DStorageErrorHandle = g_DStorageQueueRealtime->GetErrorEvent();
        (void)RegisterWaitForSingleObject(&DStorageQueueRealtimeErrorHandles.RegisteredWaitHandle
            , DStorageQueueRealtimeErrorHandles.DStorageErrorHandle
            , DStorageErrorHandler
            , g_DStorageQueueRealtime
            , INFINITE
            , WT_EXECUTEDEFAULT);

        DStorageErrorEventHandles DStorageQueueNormalErrorHandles;
        DStorageQueueNormalErrorHandles.DStorageErrorHandle = g_DStorageQueueNormal->GetErrorEvent();
        (void)RegisterWaitForSingleObject(&DStorageQueueNormalErrorHandles.RegisteredWaitHandle
            , DStorageQueueNormalErrorHandles.DStorageErrorHandle
            , DStorageErrorHandler
            , g_DStorageQueueNormal
            , INFINITE
            , WT_EXECUTEDEFAULT);

        // generate list of metadata file infos and data files.
        std::vector<FileInfo> metaDataFileInfos;
        metaDataFileInfos.reserve(g_pScenePathMap->size());
        for (const auto& pathPair : *g_pScenePathMap)
        {
            auto fileinfos{ GetSupportedFilesInfo(pathPair.second.scenePath, {"MetaData.bin"}) };
            std::copy(fileinfos.cbegin(), fileinfos.cend(), std::back_inserter(metaDataFileInfos));
        }

        std::vector<FileInfo> textureDataFileInfos;
        textureDataFileInfos.reserve(g_pScenePathMap->size());
        for (const auto& pathPair : *g_pScenePathMap)
        {
            auto fileinfos{ GetSupportedFilesInfo(pathPair.second.scenePath, {"TextureData.bin"}) };
            std::copy(fileinfos.cbegin(), fileinfos.cend(), std::back_inserter(textureDataFileInfos));
        }

        assert(metaDataFileInfos.size() > 0);
        assert(metaDataFileInfos.size() == textureDataFileInfos.size());

        // Create a structure for all metadatas.
        g_MetaDataHeaders.resize(metaDataFileInfos.size());
        for (size_t metaDataFileIdx = 0; metaDataFileIdx < metaDataFileInfos.size(); metaDataFileIdx++)
        {
            auto& metaDataHeader = g_MetaDataHeaders[metaDataFileIdx];
            auto& metaFileInfo = metaDataFileInfos[metaDataFileIdx];
            metaDataHeader.resize(metaFileInfo.Size / sizeof(DirectStorageSampleTextureMetadataHeader));
        }

        // Keeping track to close later.
        std::vector<IDStorageFile*> metaDataFileHandles;

        // Load the metadata :)
        for (size_t metaDataFileIdx = 0; metaDataFileIdx < metaDataFileInfos.size(); metaDataFileIdx++)
        {
            const auto& metaDataFile = metaDataFileInfos[metaDataFileIdx];
            DSTORAGE_REQUEST req = {};
            req.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
            req.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
            req.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MEMORY;
            req.Options.Reserved = 0;
            g_DStorageFactory->OpenFile(metaDataFile.Name.c_str(), IID_PPV_ARGS(&req.Source.File.Source));
            metaDataFileHandles.push_back(req.Source.File.Source);
            req.Source.File.Offset = 0;
            req.Source.File.Size = metaDataFile.Size;
            req.Destination.Memory.Buffer = g_MetaDataHeaders[metaDataFileIdx].data();
            req.Destination.Memory.Size = metaDataFile.Size;
            req.UncompressedSize = metaDataFile.Size;
            req.CancellationTag = 0;
            req.Name = "Read metadata";
            g_DStorageQueueRealtime->EnqueueRequest(&req);
        }
        IDStorageStatusArray* statusArray = nullptr;
        ThrowIfFailed(g_DStorageFactory->CreateStatusArray(1, "Real-time Status Array", IID_PPV_ARGS(&statusArray)));
        g_DStorageQueueRealtime->EnqueueStatus(statusArray, 0);
        g_DStorageQueueRealtime->Submit();

        while (!statusArray->IsComplete(0)) { _mm_pause(); }

        // Delete the status array. 
        statusArray->Release();

        // Gather all the resource descs to prepare for heap allocation and offset calculations.
        std::vector<std::vector<D3D12_RESOURCE_DESC>> perFileResourceDescs(g_MetaDataHeaders.size());
        for (size_t perFileResourceDescsIdx = 0; perFileResourceDescsIdx < perFileResourceDescs.size(); perFileResourceDescsIdx++)
        {
            perFileResourceDescs[perFileResourceDescsIdx].resize(g_MetaDataHeaders[perFileResourceDescsIdx].size());
            auto& resourceDescs = perFileResourceDescs[perFileResourceDescsIdx];
            for (size_t resourceDescIdx = 0; resourceDescIdx < resourceDescs.size(); resourceDescIdx++)
            {
                resourceDescs[resourceDescIdx] = g_MetaDataHeaders[perFileResourceDescsIdx][resourceDescIdx].resourceDesc;
            }
        }

        std::vector< std::vector<D3D12_RESOURCE_ALLOCATION_INFO1>> resourceAllocInfos(g_MetaDataHeaders.size());
        std::vector<D3D12_RESOURCE_ALLOCATION_INFO> fileAllocInfos(g_MetaDataHeaders.size());
        for (size_t resourceAllocsIdx = 0; resourceAllocsIdx < g_MetaDataHeaders.size(); resourceAllocsIdx++)
        {
            resourceAllocInfos[resourceAllocsIdx].resize(g_MetaDataHeaders[resourceAllocsIdx].size());
        }

        auto pathPairItr = g_pScenePathMap->cbegin();
        // Get uncompressed and on disk sizes per file. This is only being used for stats.
        for (size_t metaDataFileIdx = 0; metaDataFileIdx < metaDataFileInfos.size(); metaDataFileIdx++)
        {
            const auto& pathPair = *pathPairItr;
            const auto& assetMetaData = g_MetaDataHeaders[metaDataFileIdx];
            auto& uncompressedSize = g_SceneTextureDataSizeUncompressed[pathPair.second];
            auto& diskSize = g_SceneTextureDataSizeOnDisk[pathPair.second];
            std::for_each(assetMetaData.cbegin(), assetMetaData.cend()
                , [&uncompressedSize,&diskSize](const DirectStorageSampleTextureMetadataHeader& mdh) 
                    { uncompressedSize += mdh.resourceSizeUncompressed; diskSize += mdh.resourceSizeCompressed; });

            pathPairItr++;
        }


        ID3D12Device6* pDevice6 = nullptr;
        ThrowIfFailed(pDevice->QueryInterface(IID_PPV_ARGS(&pDevice6)));
        for (size_t perFileIdx = 0; perFileIdx < fileAllocInfos.size(); perFileIdx++)
        {
            const auto& fileResourceDescs = perFileResourceDescs[perFileIdx];
            auto& fileAllocInfo = fileAllocInfos[perFileIdx];
            auto& resourceAllocInfo = resourceAllocInfos[perFileIdx];
            fileAllocInfo = pDevice6->GetResourceAllocationInfo1(0, fileResourceDescs.size(), fileResourceDescs.data(), resourceAllocInfo.data());
        }
        pDevice6->Release();

        if (ioOptions.m_usePlacedResources)
        {
            for (size_t fileIdx = 0; fileIdx < fileAllocInfos.size(); fileIdx++)
            {
                const auto& fileAllocInfo = fileAllocInfos[fileIdx];
                //CD3DX12_HEAP_DESC
                D3D12_HEAP_DESC heapDesc{};
                heapDesc.SizeInBytes = fileAllocInfo.SizeInBytes;
                heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
                heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
                heapDesc.Properties.CreationNodeMask = 0;
                heapDesc.Properties.VisibleNodeMask = 0;

                // may need to check for UMA here.
                heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

                heapDesc.Alignment = fileAllocInfo.Alignment;
                heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES | D3D12_HEAP_FLAG_CREATE_NOT_ZEROED; // For now, allow it to be resident.

                auto scenePathMapItr = g_pScenePathMap->cbegin();
                while (scenePathMapItr != g_pScenePathMap->cend())
                {
                    const auto& scenePathPair = scenePathMapItr->second;
                    if (IsSameDirectory(metaDataFileInfos[fileIdx].Name, g_Converter.from_bytes(scenePathPair.scenePath)))
                    {
                        g_SceneHeapTemplates.insert_or_assign(scenePathPair, heapDesc);
                        break;
                    }
                    scenePathMapItr++;
                }
            }
        }

        // Close metadata file handles.
        for (auto& fileHandle : metaDataFileHandles)
        {
            fileHandle->Release();
        }


        for (size_t metaDataFileIdx = 0; metaDataFileIdx < g_MetaDataHeaders.size(); metaDataFileIdx++)
        {
            const auto& metaDataInfo = metaDataFileInfos[metaDataFileIdx];
            const auto& metaDataHeader = g_MetaDataHeaders[metaDataFileIdx];

            // Open the file handle for each texture data file.
            auto textureFileName = metaDataInfo.Name.substr(0, metaDataInfo.Name.rfind(L"MetaData.bin")) + L"TextureData.bin";
            //textureFileName.replace(textureFileName.rfind(L"MetaData.bin"), textureFileName.size(), L"TextureData.bin");

            IDStorageFile* fileHandle = nullptr;
            ThrowIfFailed(g_DStorageFactory->OpenFile(textureFileName.c_str(), IID_PPV_ARGS(&fileHandle)));
            g_FileHandles.push_back(fileHandle);

            for (size_t metaDataIdx = 0; metaDataIdx < metaDataHeader.size(); metaDataIdx++)
            {
                auto& metaDataResource = metaDataHeader[metaDataIdx];

                ResourceLookupEntry entry;
                entry.reseourceFileHandle = fileHandle;
                entry.metaDataHeader = &metaDataResource;
                entry.resourceHeapOffset = resourceAllocInfos[metaDataFileIdx][metaDataIdx].Offset;
                entry.resourceHeapSize = resourceAllocInfos[metaDataFileIdx][metaDataIdx].SizeInBytes;


                std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
                entry.gltfPath = converter.to_bytes(metaDataResource.resourceName);

                bool alreadyExists = false;
                if (!g_ResourceTable.insert_or_assign(std::wstring(metaDataResource.resourceName), entry).second)
                {
                    // This should only happen if textures have the same name. Let's see if we can get away with this.
                    assert(!"incompatible resource name found. duplicate.");
                }
            }
        }

        return true;
    }
    

    void ShutdownDirectStorage()
    {
        CPUUserMarker marker("DStorage Shutdown");
        // we could call close functions, but we are going to check our code for leaks by using Release.
        
        // Cancel any outstanding requests.
        g_DStorageQueueNormal->CancelRequestsWithTag(0, 0); 
        g_DStorageQueueRealtime->CancelRequestsWithTag(0, 0);

        // wait for everything to finish.
        DStorageSyncCPU();

        auto releaseAndCheckRefCount = [](::IUnknown* const obj)
        {
            assert(obj != nullptr);
            if (obj != nullptr)
            {
                auto refCount = obj->Release();
                assert(refCount == 0);
            }
        };

        // Close the queues, status arrays, events, etc.
        releaseAndCheckRefCount(g_DStorageQueueNormal);
        releaseAndCheckRefCount(g_DStorageQueueRealtime);
        releaseAndCheckRefCount(g_DStorageFenceGPU);
        releaseAndCheckRefCount(g_DStorageFenceCPU);

        for (auto fileHandle : g_FileHandles)
        {
            releaseAndCheckRefCount(fileHandle);
        }

        (void)CloseHandle(g_DStorageFenceCPUEvent);

        // release the factory.
        releaseAndCheckRefCount(g_DStorageFactory);
    }


    UINT64 DStorageInsertFenceCPU()
    {
        UINT64 fenceValue = ++g_DStorageFenceValueCPU;
        g_DStorageQueueNormal->EnqueueSignal(g_DStorageFenceCPU, fenceValue);
        ThrowIfFailed(g_DStorageFenceCPU->SetEventOnCompletion(fenceValue, g_DStorageFenceCPUEvent));
        return fenceValue;

    }


    static std::array<uint64_t, 512> workloads{ 0 };

    VOID NTAPI DStorageProfileMarkerFenceCallback(
        PTP_CALLBACK_INSTANCE Instance,
        PVOID                 Context,
        PTP_WAIT              Wait,
        TP_WAIT_RESULT        WaitResult
    )
    {
        (void)Instance;
        (void)Context;
        (void)Wait;
        (void)WaitResult;

        uint64_t workloadId = (uint64_t)Context;
        
        if (workloads[workloadId] == 0)
        {
            workloads[workloadId] = (uint64_t)MillisecondsNow();
        }
        else
        {
            workloads[workloadId] = (uint64_t)MillisecondsNow() - workloads[workloadId];
        }
    }

    // Need a signal to say work started.
    uint64_t DStorageBeginProfileLoading() // return workload id.
    {
        // Generate a work id and insert it into outstanding workload queue
        ++g_DStorageFenceValueProfile;
        uint64_t workloadId = g_DStorageFenceValueProfile % workloads.size();
        workloads[workloadId] = 0;

        // Queue up an event for the start of work.
        IDStorageQueue1* dstorageQueue1 = nullptr;
        if (SUCCEEDED(g_DStorageQueueNormal->QueryInterface(IID_PPV_ARGS(&dstorageQueue1))))
        {
            auto pTPWait = CreateThreadpoolWait(&DStorageProfileMarkerFenceCallback, (void*)workloadId, nullptr);
            SetThreadpoolWait(pTPWait, g_DStorageFenceProfileEvent, nullptr);
            dstorageQueue1->EnqueueSetEvent(g_DStorageFenceProfileEvent);

            dstorageQueue1->Release();
        }

        return workloadId;
    }



    // Need a signal to say this work ended.
    void DStorageEndProfileLoading(size_t workloadId) // inserts another event to signal end of loading.
    {
        // Queue up an event for the start of work.
        IDStorageQueue1* dstorageQueue1 = nullptr;
        if (SUCCEEDED(g_DStorageQueueNormal->QueryInterface(IID_PPV_ARGS(&dstorageQueue1))))
        {
            auto pTPWait = CreateThreadpoolWait(&DStorageProfileMarkerFenceCallback, (void*)workloadId, nullptr);
            SetThreadpoolWait(pTPWait, g_DStorageFenceProfileEvent, nullptr);

            dstorageQueue1->EnqueueSetEvent(g_DStorageFenceProfileEvent);

            dstorageQueue1->Release();
        }
    }

    // Need a way to retrieve the value for the work id.
    uint64_t DStorageProfileRetrieveTiming(uint64_t workloadId)
    {
        return workloads[workloadId];
    }


    void DStorageSyncCPU(uint64_t fenceValue)
    {
        CPUUserMarker marker("DStorageSyncCPU: Waiting for DS to complete on CPU... ");
        while (g_DStorageFenceCPU->GetCompletedValue() < fenceValue)
        {
            (void)WaitForSingleObject(g_DStorageFenceCPUEvent, INFINITE);
        }
    }

    void DStorageSyncCPU()
    {
        CPUUserMarker marker("DStorageSyncCPU: Waiting for DS to complete on CPU... ");
        UINT64 fenceValue = DStorageInsertFenceCPU();
        ThrowIfFailed(g_DStorageFenceCPU->SetEventOnCompletion(fenceValue, g_DStorageFenceCPUEvent));
        g_DStorageQueueNormal->Submit();

        while (g_DStorageFenceCPU->GetCompletedValue() < fenceValue)
        {
            (void)WaitForSingleObject(g_DStorageFenceCPUEvent, INFINITE);
        }
    }

    void DStorageSyncGPU(ID3D12CommandQueue* queue)
    {
        CPUUserMarker marker("DStorageSyncCPU: Waiting for DS to complete on GPU... ");
        UINT64 fenceValue = ++g_DStorageFenceValueGPU;
        g_DStorageQueueNormal->EnqueueSignal(g_DStorageFenceGPU, fenceValue);
        g_DStorageQueueNormal->Submit();
        ThrowIfFailed(queue->Wait(g_DStorageFenceGPU, fenceValue));
    }

    void DStorageSubmit()
    {
        CPUUserMarker marker("DStorageSubmit");
        g_DStorageQueueNormal->Submit();
    }

    void DStorageCancelRequest(uint64_t workloadId)
    {
        g_DStorageQueueNormal->CancelRequestsWithTag(UINT64_MAX, workloadId);
    }
}
