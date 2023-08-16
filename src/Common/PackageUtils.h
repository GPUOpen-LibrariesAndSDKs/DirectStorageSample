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

#pragma once

#include "json.h"

struct FileInfo
{
    std::wstring Name;
    uint64_t Size;
};

std::vector<std::wstring> GetGLTFTexturePaths(const nlohmann::json& gltfJson);
std::map<std::wstring, std::vector<std::wstring>> GetGLTFPathFileMapping();
std::wstring GetFullDirectoryPath(const std::wstring& dir);
std::wstring GetFileName(const std::wstring& path);
bool IsSameDirectory(const std::wstring& dir1, const std::wstring& dir2);
std::vector<FileInfo> GetSupportedFilesInfo(const std::wstring& basePath, const std::vector<std::wstring>& searchStrings);
std::vector<FileInfo> GetSupportedFilesInfo(const std::string& basePath, const std::vector<std::string>& searchStrings);


