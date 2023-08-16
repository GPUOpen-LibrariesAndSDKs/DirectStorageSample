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
#include "Misc/ImgLoader.h"
#include "Misc/DDSLoader.h"
#include "Misc/WICLoader.h"
#include <dstorage.h> // using for compression codec.
#include "DirectStorageSampleTexturePackageFormat.h"
#include "PackageUtils.h"
#include "CompressionSupport.h"
#include <codecvt>
#include "json.h"
#include <fstream>
#include <thread>


using Microsoft::WRL::ComPtr;

bool ConvertImages(ID3D12Device* const pDevice, const std::wstring& gltfPath, const std::vector<std::wstring>& imageList, DSTORAGE_COMPRESSION_FORMAT compressionFormat, DSTORAGE_COMPRESSION compressionLevel, bool compressionExhaustive);
int64_t Compress(DSTORAGE_COMPRESSION_FORMAT format, DSTORAGE_COMPRESSION compressionLevel, std::vector<uint8_t>& compressedDst, const std::vector<uint8_t>& uncompressedSrc);



std::vector<std::wstring> GetSupportedFiles(const std::wstring& path)
{

    std::vector<std::wstring> fileNames;

    const wchar_t* supportedFileTypes[] =
    {
        L"*.dds"
        ,L"*.jpeg"
        ,L"*.jpg"
        ,L"*.png"
        ,L"*.gif"
        ,L"*.bmp"
    };

    for (auto& fileType : supportedFileTypes)
    {
        WIN32_FIND_DATAW findData;
        HANDLE searchHandle = FindFirstFileExW((path + fileType).c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
        if (searchHandle != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    fileNames.push_back(path + findData.cFileName);
                }
            } while (FindNextFileW(searchHandle, &findData));
        }
        FindClose(searchHandle);
    }

    return fileNames;
}


static bool ValidateCompressionArgs(const std::wstring& compressionFormatString, const std::wstring& compressionLevelString, const std::wstring& compressionExhaustiveString)
{
    if (TranslateCompressionExhaustiveToString(TranslateCompressionExhaustiveToValue(compressionExhaustiveString)) == compressionExhaustiveString)
    {
        // All other options are ignored.
        return true;
    }

    auto compressionFormatValue = TranslateCompressionFormatToValue(compressionFormatString);
    if (TranslateCompressionFormatToString(compressionFormatValue) != compressionFormatString)
    {
        return false;
    }

    if (compressionFormatValue == DSTORAGE_COMPRESSION_FORMAT_GDEFLATE)
    {
        if (TranslateCompressionLevelToStringGDeflate(TranslateCompressionLevelToValueGDeflate(compressionLevelString)) != compressionLevelString)
        {
            return false;
        }
    }

    return true;
}


static const std::wstring& GetUsageString()
{
    static const std::wstring usageString(L""
    L"Usage: TextureConverter.exe -configFile=<path to DirectStorageSample.json> -compressionFormat=<Compression Format> [-compressionLevel=<Valid Compression Level>] [-compressionExhaustive=<false|true>]"
    L"\n"
    L"Compression Formats:\n"
    L"\tnone\n"
    L"\tgdeflate\n"
    L"\n"
    L"Compression Level:\n"
    L"\tdefault (balance between compression ratio and compression performance)\n"
    L"\tfastest (fastest to compress, but potentially lower compression ratio)\n"
    L"\tbest (highest compression ratio)\n"
    L"\n"
    L"Compression Exhaustive:\n"
    L"\tfalse (use the compressionLevel and compressionFormat specified -- default)\n"
    L"\ttrue (Use the compression format and compression level with the best compression ratio. compressionLevel and compressionFormat specified are ignored)\n"
    L"\n"
    );

    return usageString;
}


int wmain(int argc, wchar_t* argv[])
{
    std::wstring configFile(L"");
    std::wstring compressionFormatString(L"");
    std::wstring compressionLevelString(L"default");
    std::wstring compressionExhaustiveString(L"");
    DSTORAGE_COMPRESSION_FORMAT compressionFormatValue = DSTORAGE_COMPRESSION_FORMAT_NONE;
    DSTORAGE_COMPRESSION compressionLevelValue = DSTORAGE_COMPRESSION_DEFAULT;
    bool compressionExhaustiveValue = false;

    // Parse command-line args.
    for (int argIdx = 0; argIdx < argc; argIdx++)
    {
        if (argv[argIdx][0] == L'-')
        {
            // command-line option
            wchar_t* argValPtr = nullptr;
            if ((argValPtr = wcsstr(&argv[argIdx][1], L"configFile=")) != nullptr)
            {
                configFile = std::wstring(wcschr(argValPtr, L'=') + 1);
                continue;
            }

            if ((argValPtr = wcsstr(&argv[argIdx][1], L"compressionFormat=")) != nullptr)
            {
                compressionFormatString = std::wstring(wcschr(argValPtr, L'=') + 1);
                continue;
            }
        
            if ((argValPtr = wcsstr(&argv[argIdx][1], L"compressionLevel=")) != nullptr)
            {
                compressionLevelString = std::wstring(wcschr(argValPtr, L'=') + 1);
                continue;
            }

            if ((argValPtr = wcsstr(&argv[argIdx][1], L"compressionExhaustive=")) != nullptr)
            {
                compressionExhaustiveString = std::wstring(wcschr(argValPtr, L'=') + 1);
                continue;
            }
        }
    }

    if (!ValidateCompressionArgs(compressionFormatString, compressionLevelString, compressionExhaustiveString))
    {
        std::wcerr << "Invalid arguments." << std::endl << GetUsageString();

        // bail.
        return -1;
    }


    if (compressionExhaustiveString != L"")
    {
        compressionExhaustiveValue = TranslateCompressionExhaustiveToValue(compressionExhaustiveString);
    }

    if (compressionExhaustiveValue == false)
    {
        // Setup compression settings.
        if (compressionFormatString != L"")
        {
            compressionFormatValue = TranslateCompressionFormatToValue(compressionFormatString);

            if (compressionFormatValue == DSTORAGE_COMPRESSION_FORMAT_GDEFLATE)
            {
                compressionLevelValue = TranslateCompressionLevelToValueGDeflate(compressionLevelString);
            }
        }

        std::wcout << L"Compression Format: " << TranslateCompressionFormatToString(compressionFormatValue) << std::endl << L"Compression Level: " << TranslateCompressionLevelToStringGDeflate(compressionLevelValue) << std::endl;
    }
    else
    {
        std::wcout << L"Compression exhaustive search enabled." << std::endl;
    }

    
    if (!PathFileExistsW(configFile.c_str()))
    {
        std::wcerr << L"Config file path does not exist: " << configFile << std::endl;
        return -1;
    }


    std::wstring configPath(GetFullDirectoryPath(configFile));
    SetCurrentDirectoryW(configPath.c_str());

    std::unordered_map<std::wstring, std::vector<std::wstring>> gltfRelativePaths;

    // Read in GLTF path definitions from config file.
    auto configFileNameOnly{ GetFileName(configFile) };
    std::ifstream configStream(configFileNameOnly,std::ios::in,std::ios::binary);
    nlohmann::json config;
    configStream >> config;
    const auto& scenes = config["scenes"];
    std::vector<std::wstring> gltfFilePaths;
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utf8utf16converter;
    for (const auto& scene : scenes)
    {
        const auto& sceneName = scene["name"].get<std::string>();
        const auto& sceneDirectory = scene["directory"].get<std::string>();
        const auto& sceneFilename = scene["filename"].get<std::string>();
        auto scenePathWstr = utf8utf16converter.from_bytes(sceneDirectory + sceneFilename);
        if (PathFileExistsW(scenePathWstr.c_str()))
        {
            gltfFilePaths.push_back(std::move(scenePathWstr));
        }
        else
        {
            std::wcerr << L"Scene: " << utf8utf16converter.from_bytes(sceneName) << L" does not exist at location: " << scenePathWstr << std::endl;
        }
    }

    // Grab all texture paths.
    std::cout << "Reading texture paths for..." << std::endl;
    for (const auto& gltfPath : gltfFilePaths)
    {
        std::wcout << gltfPath << std::endl;
        std::ifstream jsonStream(gltfPath,std::ios::in|std::ios::binary);
        nlohmann::json j;
        jsonStream >> j;
        gltfRelativePaths[gltfPath] = GetGLTFTexturePaths(j);
    }


    // Scan for supported file types.
    if (gltfRelativePaths.size() == 0)
    {
        std::cerr << "No input textures found.";
        return -1;
    }
     
    ComPtr<ID3D12Device> pDevice;
    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&pDevice))) )
    {
        std::wcerr << L"Failure to create device.\n";
        return -1;
    }


    std::wcout << L"Converting textures for..." << std::endl;
    for (const auto& gltfRelativePath : gltfRelativePaths)
    {
        // Resolve to full path before conversion?
        std::wcout << gltfRelativePath.first << std::endl;
        if (!ConvertImages(pDevice.Get(), gltfRelativePath.first, gltfRelativePath.second,compressionFormatValue, compressionLevelValue, compressionExhaustiveValue))
        {
            std::wcerr << L"Failure to convert images for..." << gltfRelativePath.first << std::endl;
        }
    }
}

bool CreateFileOnDisk(const wchar_t* const path, HANDLE* handleInOut)
{
    assert(handleInOut != nullptr);

    // Create files if necessary.
    if (*handleInOut == INVALID_HANDLE_VALUE)
    {
        *handleInOut = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            SetLastError(ERROR_SUCCESS); // ignore if already exist.
        }
        else if (GetLastError() != ERROR_SUCCESS)
        {
            std::wcerr << L"Failure to open file: " << path << std::endl;
            return false;
        }
    }

    return true;
}

int64_t WriteDataToDisk(const HANDLE fileHandle, const void* const data, const size_t byteCount)
{
    LARGE_INTEGER filePointerOrStatus{ 0 };
    filePointerOrStatus.LowPart = SetFilePointer(fileHandle, 0, &filePointerOrStatus.HighPart, FILE_CURRENT);

    size_t totalBytesWritten = 0;
    const char* dataPtr = (char*) data;
    size_t bytesRemaining = byteCount;

    while (bytesRemaining > 0)
    {
        DWORD bytesToWrite = static_cast<DWORD>(min(bytesRemaining, MAXDWORD));
        DWORD bytesWrittenThisCall = 0;
        if (WriteFile(fileHandle, dataPtr + totalBytesWritten, bytesToWrite, &bytesWrittenThisCall, NULL))
        {
            bytesRemaining -= bytesWrittenThisCall;
            dataPtr += bytesWrittenThisCall;
        }
        else
        {
            return -1;
        }
    }

    // Return prior offset in current file so we know where the data is located.
    return filePointerOrStatus.QuadPart;
}

#if 1
// Find best compressino for the given asset.
int64_t CompressExhaustive(std::vector<uint8_t>& compressedDst, const std::vector<uint8_t>& uncompressedSrc, DSTORAGE_COMPRESSION_FORMAT* formatOut)
{
    // @todo this much be updated as formats and levels are added.
    DSTORAGE_COMPRESSION_FORMAT supportedFormatMax = DSTORAGE_COMPRESSION_FORMAT_GDEFLATE;
    DSTORAGE_COMPRESSION_FORMAT supportedFormatMin = DSTORAGE_COMPRESSION_FORMAT_NONE;
    DSTORAGE_COMPRESSION supportedFormatLevelMax = DSTORAGE_COMPRESSION_BEST_RATIO;
    DSTORAGE_COMPRESSION supportedFormatLevelMin = DSTORAGE_COMPRESSION_FASTEST;

    int64_t smallestSize = INT64_MAX;
    
    std::vector<uint8_t> tempCompressedBuffer;

    for (std::underlying_type<DSTORAGE_COMPRESSION_FORMAT>::type format = supportedFormatMin; format <= supportedFormatMax; format++)
    {
        for (std::underlying_type<DSTORAGE_COMPRESSION>::type level = supportedFormatLevelMin; level <= supportedFormatLevelMax; level++)
        {
            int64_t compressedSize  = Compress(static_cast<DSTORAGE_COMPRESSION_FORMAT>(format), static_cast<DSTORAGE_COMPRESSION>(level), tempCompressedBuffer, uncompressedSrc);
            if (compressedSize < smallestSize)
            {
                smallestSize = compressedSize;
                compressedDst = std::move(tempCompressedBuffer);
                *formatOut = static_cast<DSTORAGE_COMPRESSION_FORMAT>(format);
            }
        }
    }
    return smallestSize;
}
#endif

int64_t Compress(DSTORAGE_COMPRESSION_FORMAT format, DSTORAGE_COMPRESSION compressionLevel, std::vector<uint8_t>& compressedDst, const std::vector<uint8_t>& uncompressedSrc)
{
    if (format != DSTORAGE_COMPRESSION_FORMAT_NONE)
    {
        ComPtr<IDStorageCompressionCodec> codec;
        if (FAILED(DStorageCreateCompressionCodec(format, std::thread::hardware_concurrency(), IID_PPV_ARGS(&codec))))
        {
            std::wcerr << L"Unable to create compression codec. Check compression format or library version.";
            return -1;
        }

        auto compressedBytesMax = codec->CompressBufferBound(uncompressedSrc.size());
        compressedDst.resize(compressedBytesMax);
        
        // Note: For now we assume that compression is a benefit, but it might not be. It's best to check the actual compressed size and make a decision.
        size_t compressedBytesActual = 0;
        if (FAILED(codec->CompressBuffer(uncompressedSrc.data(), uncompressedSrc.size(), compressionLevel, compressedDst.data(), compressedDst.size(), &compressedBytesActual)))
        {
            std::wcerr << L"Compression failure.";
            return -1;
        }

        return compressedBytesActual;
    }
    else
    {
        if (compressedDst.size() < uncompressedSrc.size())
        {
            compressedDst.resize(uncompressedSrc.size());
        }
        memcpy(compressedDst.data(), uncompressedSrc.data(), uncompressedSrc.size());
        return uncompressedSrc.size();
    }   
}


bool ConvertImages(ID3D12Device* const pDevice, const std::wstring& gltfPath, const std::vector<std::wstring>& imageList, DSTORAGE_COMPRESSION_FORMAT compressionFormat, DSTORAGE_COMPRESSION compressionLevel, bool compressionExhaustive)
{

    HANDLE metadataFileHandle = INVALID_HANDLE_VALUE;
    HANDLE texturedataFileHandle = INVALID_HANDLE_VALUE;

    ImgLoader* imgLoader = nullptr;

    std::vector<wchar_t> gltfPathWithoutFilename(gltfPath.begin(), gltfPath.end());
    gltfPathWithoutFilename.resize(gltfPathWithoutFilename.size() + 2, 0); // +1 for blackslash +1 for '\0'
    if (PathRemoveFileSpecW(gltfPathWithoutFilename.data()))
    {
        PathAddBackslashW(gltfPathWithoutFilename.data());
    }

    for (auto& imageName : imageList)
    {
        std::wstring gltfRelativeImagePath = std::wstring(gltfPathWithoutFilename.data()) + imageName;

        IMG_INFO info;

        // Read in image file.
        std::wstring upperCaseImageName(gltfRelativeImagePath);
        std::transform(gltfRelativeImagePath.begin(), gltfRelativeImagePath.end(), upperCaseImageName.begin(), [](const wchar_t& a) { return std::toupper(a); });
        if (upperCaseImageName.rfind(L".DDS") != std::string::npos)
        {
            imgLoader = new DDSLoader;
        }
        else
        {
            imgLoader = new WICLoader;
        }


        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
        if (imgLoader->Load(converter.to_bytes(gltfRelativeImagePath.c_str()).c_str(), 0.0f, &info))
        {

        }
        else
        {
            std::wcerr << "Failure to load file: " << gltfRelativeImagePath << std::endl;
            continue;
        }

        // Create resource desc.
        UINT subresourceCount = max(info.arraySize, info.depth) * info.mipMapCount;
        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = info.depth > 1 ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = info.width;
        resourceDesc.Height = info.height;
        resourceDesc.DepthOrArraySize = max(info.arraySize, info.depth);
        resourceDesc.MipLevels = info.mipMapCount;
        resourceDesc.Format = info.format;
        resourceDesc.SampleDesc = { 1, 0 };
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> subresourceFootprints(subresourceCount);
        std::vector<UINT> subresourceRowsCount(subresourceCount);
        std::vector<UINT64> subresourceRowByteCount(subresourceCount);
        UINT64 subresourceTotalByteCount = 0;

        // Determine layout for disk.
        pDevice->GetCopyableFootprints(&resourceDesc
            , 0, subresourceCount
            , 0, &subresourceFootprints[0]
            , &subresourceRowsCount[0]
            , &subresourceRowByteCount[0]
            , &subresourceTotalByteCount);

        // Allocate memory to copy into.
        std::vector<uint8_t> textureData(subresourceTotalByteCount, 0);

        // copy texture data...
        for (UINT subResourceIdx = 0; subResourceIdx < subresourceCount; subResourceIdx++)
        {
            // Src setup
            size_t srcRowPitchBytes = ((info.bitCount * info.width) + 7) / 8; // rounded to nearest byte.
            size_t srcSlicePitchBytes = srcRowPitchBytes * info.height; // assuming tightly packed image.

            // Dst setup
            const auto& resourceFootprint = subresourceFootprints[subResourceIdx];
            auto resourcePtr = textureData.data() + resourceFootprint.Offset;
            const auto& footprint = resourceFootprint.Footprint;
            size_t dstRowPitchBytes = resourceFootprint.Footprint.RowPitch; // padded row pitch.
            size_t dstRowPitchPackedBytes = subresourceRowByteCount[subResourceIdx];
            size_t dstSlicePitchBytes = subresourceRowsCount[subResourceIdx] * dstRowPitchBytes;

            size_t resolvedHeight = min(subresourceRowsCount[subResourceIdx], info.height);
            size_t resolvedPackedRowPitch = min(min(dstRowPitchBytes, srcRowPitchBytes), dstRowPitchPackedBytes);

            //for (size_t y = 0; y < resolvedHeight; y++)
            {
                // going to be lame here and take the src row pitch from dst row pitch because we don't have the info.
                imgLoader->CopyPixels(resourcePtr, dstRowPitchBytes, resolvedPackedRowPitch, resolvedHeight);
            }
        }

        // Create required files.
        if (!CreateFileOnDisk((std::wstring(gltfPathWithoutFilename.data()) + (L"MetaData.bin")).c_str(), &metadataFileHandle))
        {
            return false;
        }

        if (!CreateFileOnDisk((std::wstring(gltfPathWithoutFilename.data()) + (L"TextureData.bin")).c_str(), &texturedataFileHandle))
        {
            return false;
        }


        std::vector<uint8_t> gpuData;
        int64_t gpuDataSize = -1;
        if (compressionExhaustive)
        {
            gpuDataSize = CompressExhaustive(gpuData, textureData, &compressionFormat);
        }
        else if (compressionFormat != DSTORAGE_COMPRESSION_FORMAT_NONE)
        {
            // compression enabled.
            gpuData.resize(textureData.size());
            gpuDataSize = Compress(compressionFormat, compressionLevel, gpuData, textureData);
            if (gpuDataSize == -1)
            {
                std::wcerr << "Failed to compress image: " << gltfRelativeImagePath << std::endl;
                return false;
            }

            if (textureData.size() <= gpuDataSize)
            {
                // Turns out compression didn't help us at all. TODO: Determine threshold at which compression should be disabled.
                std::wcout << "Compression ineffective for " << gltfRelativeImagePath << std::endl;
            }
        }
        else
        {
            // no compression... avoiding allocation.
            gpuData = std::move(textureData);
            gpuDataSize = gpuData.size();
            if (gpuDataSize == 0)
            {
                return false;
            }
        }

        // Write GPU Data and obtain offset to data.
        int64_t textureDataOffsetOnDisk = WriteDataToDisk(texturedataFileHandle, gpuData.data(), gpuDataSize);
        
        // Assemble metadata.
        DirectStorageSampleTextureMetadataHeader metadata;
        metadata.resourceDesc = resourceDesc;
        metadata.resourceSizeCompressed = gpuDataSize; // will be same as uncompressed size without compression.
        metadata.resourceSizeUncompressed = subresourceTotalByteCount;
        metadata.compressionFormat = compressionFormat;
        wcsncpy(metadata.resourceName, gltfRelativeImagePath.c_str(), std::extent_v<decltype(metadata.resourceName)> - 1);
        metadata.resourceName[std::extent_v<decltype(metadata.resourceName)> - 1] = '\0'; // ensure truncation.
        metadata.resourceOffset = textureDataOffsetOnDisk;
        assert((textureDataOffsetOnDisk % 4096) == 0);

        // Write CPU Data.
        WriteDataToDisk(metadataFileHandle, &metadata, sizeof(metadata));

        // Align next write for Texture data.

        // now align the data..
        int64_t unalignedOffset = WriteDataToDisk(texturedataFileHandle, nullptr, 0);
        char zeroData[4096];
        int64_t dataAlignmentBytes = ((unalignedOffset + 4095) & (~4096 + 1))- unalignedOffset;
        (void)WriteDataToDisk(texturedataFileHandle, zeroData, dataAlignmentBytes);

        delete imgLoader;
    }

    CloseHandle(metadataFileHandle);
    CloseHandle(texturedataFileHandle);

    return true;
}
