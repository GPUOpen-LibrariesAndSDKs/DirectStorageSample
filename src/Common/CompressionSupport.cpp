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

#include "CompressionSupport.h"
#include <dstorage.h>
#include <array>

template<typename T, typename U>
struct StringValuePair
{
    T argString;
    U argValue;
};

using CompressionFormatArgPair = StringValuePair<std::wstring, DSTORAGE_COMPRESSION_FORMAT>;

static const std::array<CompressionFormatArgPair, 2> s_supportedComrpessionFormats{
     CompressionFormatArgPair{std::wstring(L"none"),DSTORAGE_COMPRESSION_FORMAT_NONE}
    ,CompressionFormatArgPair{std::wstring(L"gdeflate"),DSTORAGE_COMPRESSION_FORMAT_GDEFLATE}
};

const std::wstring& TranslateCompressionFormatToString(DSTORAGE_COMPRESSION_FORMAT compressionFormatValue)
{
    for (const auto& compressionFormat : s_supportedComrpessionFormats)
    {
        if (compressionFormat.argValue == compressionFormatValue)
        {
            return compressionFormat.argString;
        }
    }

    return s_supportedComrpessionFormats[0].argString;
}

DSTORAGE_COMPRESSION_FORMAT TranslateCompressionFormatToValue(const std::wstring& compressionFormatString)
{
    for (const auto& compressionFormat : s_supportedComrpessionFormats)
    {
        if (compressionFormat.argString.size() == compressionFormatString.size())
        {
            if (compressionFormat.argString == compressionFormatString)
            {
                return compressionFormat.argValue;
            }
        }
    }

    return s_supportedComrpessionFormats[0].argValue;
}

using CompressionLevelArgPair = StringValuePair<std::wstring, DSTORAGE_COMPRESSION>;

static const std::array<CompressionLevelArgPair, 4> s_supportedCompressionLevelsGDeflate{
     CompressionLevelArgPair{std::wstring(L"default"),DSTORAGE_COMPRESSION_DEFAULT}
    ,CompressionLevelArgPair{std::wstring(L"fastest"),DSTORAGE_COMPRESSION_FASTEST}
    ,CompressionLevelArgPair{std::wstring(L"best"),DSTORAGE_COMPRESSION_BEST_RATIO}
};

const std::wstring& TranslateCompressionLevelToStringGDeflate(DSTORAGE_COMPRESSION compressionLevelValue)
{
    for (const auto& compressionLevel : s_supportedCompressionLevelsGDeflate)
    {
        if (compressionLevel.argValue == compressionLevelValue)
        {
            return compressionLevel.argString;
        }
    }

    return s_supportedCompressionLevelsGDeflate[0].argString;
}

DSTORAGE_COMPRESSION TranslateCompressionLevelToValueGDeflate(const std::wstring& compressionLevelString)
{
    for (const auto& compressionLevel : s_supportedCompressionLevelsGDeflate)
    {
        if (compressionLevel.argString.size() == compressionLevelString.size())
        {
            if (compressionLevel.argString == compressionLevelString)
            {
                return compressionLevel.argValue;
            }
        }
    }

    return s_supportedCompressionLevelsGDeflate[0].argValue;
}


using CompressionExhuastiveArgPair = StringValuePair<std::wstring, bool>;
static const std::array<CompressionExhuastiveArgPair, 4> s_supportedCompressionExhaustive{
     CompressionExhuastiveArgPair{std::wstring(L"false"),false}
    ,CompressionExhuastiveArgPair{std::wstring(L"true"),true}
    ,CompressionExhuastiveArgPair{std::wstring(L"0"),false}
    ,CompressionExhuastiveArgPair{std::wstring(L"1"),true}
};


const std::wstring& TranslateCompressionExhaustiveToString(bool compressionExhaustiveValue)
{
    for (const auto& compressionExhaustivePair : s_supportedCompressionExhaustive)
    {
        if (compressionExhaustivePair.argValue == compressionExhaustiveValue)
        {
            return compressionExhaustivePair.argString;
        }
    }

    return s_supportedCompressionExhaustive[0].argString;
}

bool TranslateCompressionExhaustiveToValue(const std::wstring& compressionExhaustiveString)
{
    for (const auto& compressionExhaustivePair : s_supportedCompressionExhaustive)
    {
        if (compressionExhaustivePair.argString.size() == compressionExhaustiveString.size())
        {
            if (compressionExhaustivePair.argString == compressionExhaustiveString)
            {
                return compressionExhaustivePair.argValue;
            }
        }
    }

    return s_supportedCompressionExhaustive[0].argValue;
}
