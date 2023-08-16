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

struct IOOptions;

namespace Sample
{

    bool InitializeDirectStorage(ID3D12Device* const pDevice, const std::wstring& contentPathRoot, const std::unordered_map<std::string, ScenePathPair>& scenePathMap, const IOOptions& sampleOptions);
    void ShutdownDirectStorage();

    uint64_t DStorageBeginProfileLoading();

    void DStorageEndProfileLoading(size_t workloadId);

    uint64_t DStorageProfileRetrieveTiming(uint64_t workloadId);

    UINT64 DStorageInsertFenceCPU();
    void DStorageSyncCPU();

    void DStorageSyncCPU(uint64_t fenceValue);
    void DStorageSyncGPU(ID3D12CommandQueue* queue);

    void DStorageSubmit();

    void DStorageCancelRequest(uint64_t workloadId);

    D3D12_HEAP_DESC GetTextureHeapDescForScene(const ScenePathPair& scenePathPair);

    size_t GetSceneTextureDataSizeOnDisk(const ScenePathPair& scenePathPair);
    size_t GetSceneTextureDataSizeUncompressed(const ScenePathPair& scenePathPair);
    double GetSceneTextureCompressionRatio(const ScenePathPair& scenePathPair);
   
   // This class provides functionality to create a 2D-texture from a DDS or any texture format from WIC file.
    class Texture:public ::CAULDRON_DX12::Texture
    {
    public:
        virtual bool InitFromFile(Device* pDevice, UploadHeap* pUploadHeap, ID3D12Heap* pTextureHeap, const char* szFilename, uint64_t workloadId, bool useSRGB = false, float cutOff = 1.0f, D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE);
    };

    #include "GLTF/GLTFTexturesAndBuffersDecl.inl"
}
