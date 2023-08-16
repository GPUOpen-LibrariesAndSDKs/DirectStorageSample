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
// THE SOFTWARE

#include "json.h"
#include <codecvt>
#include <shlwapi.h>
#include <stack>
#include <fstream>
#include <iostream>
#include "PackageUtils.h"


using namespace nlohmann;

std::vector<std::wstring> GetGLTFTexturePaths(const nlohmann::json& gltfJson)
{
    std::vector<std::wstring> paths;
    // load textures 
    if (gltfJson.find("images") != gltfJson.end())
    {
        auto pTextureNodes = &gltfJson["textures"];
        const json& images = gltfJson["images"];
        const json& materials = gltfJson["materials"];
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;

        for (int imageIndex = 0; imageIndex < images.size(); imageIndex++)
        {
            std::wstring filename{ converter.from_bytes(images[imageIndex]["uri"].get<std::string>()) };
            paths.emplace_back(filename);
        }
    }

    return paths;
}

std::map<std::wstring,std::vector<std::wstring>> GetGLTFPathFileMapping()
{
    std::map<std::wstring, std::vector<std::wstring>> gltfRelativePaths;

    std::ifstream configStream(L"DirectStorageSample.json", std::ios::in, std::ios::binary);
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
        const auto& scenePath = sceneDirectory + sceneFilename;
        auto scenePathWstr = utf8utf16converter.from_bytes(scenePath);
        if (PathFileExistsW(scenePathWstr.c_str()))
        {
            gltfFilePaths.push_back(std::move(scenePathWstr));
        }
        else
        {
            std::wcerr << L"Scene: " << utf8utf16converter.from_bytes(sceneName) << L"does not exist at location: " << scenePathWstr << std::endl;
        }
    }

    // Grab all texture paths.
    for (const auto& gltfPath : gltfFilePaths)
    {
        std::ifstream jsonStream(gltfPath, std::ios::in | std::ios::binary);
        nlohmann::json j;
        jsonStream >> j;
        gltfRelativePaths[gltfPath] = GetGLTFTexturePaths(j);
    }

    return gltfRelativePaths;
}


std::wstring GetFileName(const std::wstring& path)
{
    DWORD fullPathRequiredSize = GetFullPathNameW(path.c_str(), NULL, NULL, NULL);
    std::vector<wchar_t> fullPath(fullPathRequiredSize, L'0');
    wchar_t* fullPathFilePart = nullptr;
    GetFullPathNameW(path.c_str(), fullPath.size(), fullPath.data(), &fullPathFilePart);

    if (fullPathFilePart)
    {
        return std::wstring(fullPathFilePart);
    }

    return std::wstring();
}

std::wstring GetFullDirectoryPath(const std::wstring& dir)
{
    DWORD fullPathRequiredSize = GetFullPathNameW(dir.c_str(), NULL, NULL, NULL);
    std::vector<wchar_t> fullPath(fullPathRequiredSize, L'0');
    wchar_t* fullPathFilePart = nullptr;
    GetFullPathNameW(dir.c_str(), fullPath.size(), fullPath.data(), &fullPathFilePart);
    if (fullPathFilePart)
    {
        *fullPathFilePart = L'\0';
    }

    return std::wstring(fullPath.data());
}

bool IsSameDirectory(const std::wstring& dir1, const std::wstring& dir2)
{
    auto fullDirPath1{ GetFullDirectoryPath(dir1) };
    auto fullDirPath2{ GetFullDirectoryPath(dir2) };

    return std::wcscmp(fullDirPath1.c_str(), fullDirPath2.c_str()) == 0;
}

std::vector<std::wstring> GetPathsBelow(const std::wstring& basePath)
{
    std::vector<std::wstring> foundPaths;
    std::stack<std::wstring> pathQueue;
    foundPaths.push_back(basePath);
    pathQueue.push(basePath);
    // find all directories below the base path.

    while (!pathQueue.empty())
    {
        const auto searchPath = pathQueue.top();
        pathQueue.pop();

        WIN32_FIND_DATAW findData;
        HANDLE searchHandle = FindFirstFileExW((searchPath + L"*").c_str(), FindExInfoBasic, &findData, FindExSearchLimitToDirectories, NULL, FIND_FIRST_EX_LARGE_FETCH);
        if (searchHandle != INVALID_HANDLE_VALUE)
        {
            do
            {
                if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0)
                    {
                        std::wstring foundPath = searchPath + findData.cFileName + L"\\";
                        foundPaths.push_back(foundPath);
                        pathQueue.push(std::move(foundPath));
                    }
                }

            } while (FindNextFileW(searchHandle, &findData));
        }
        FindClose(searchHandle);
    }

    return foundPaths;

}

std::vector<FileInfo> GetSupportedFilesInfo(const std::wstring& basePath, const std::vector<std::wstring>& searchStrings)
{
    std::vector<FileInfo> fileInfos;
    std::vector<std::wstring> searchPaths;

    searchPaths = GetPathsBelow(basePath);

    for (const auto& searchPath : searchPaths)
    {
        for (auto& searchString : searchStrings)
        {
            WIN32_FIND_DATAW findData;
            HANDLE searchHandle = FindFirstFileExW((searchPath + searchString).c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
            if (searchHandle != INVALID_HANDLE_VALUE)
            {
                do
                {
                    if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                    {
                        std::wstring foundPath = searchPath + findData.cFileName;
                        fileInfos.push_back({ std::move(foundPath),findData.nFileSizeHigh | findData.nFileSizeLow });
                    }
                } while (FindNextFileW(searchHandle, &findData));
            }
            FindClose(searchHandle);
        }
    }

    return fileInfos;
}

std::vector<FileInfo> GetSupportedFilesInfo(const std::string& basePath, const std::vector<std::string>& searchStrings)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utf8utf16converter;

    std::vector<std::wstring> searchStringsW(searchStrings.size());
    std::transform(searchStrings.begin(), searchStrings.end(), searchStringsW.begin(), [&utf8utf16converter](const std::string& s) {return utf8utf16converter.from_bytes(s); });

    return GetSupportedFilesInfo(utf8utf16converter.from_bytes(basePath), searchStringsW);
}