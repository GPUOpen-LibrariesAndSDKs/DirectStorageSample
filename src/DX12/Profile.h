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

#include <deque>
#include <string>
#include <inttypes.h>
#include <fcntl.h>
#include <io.h>

namespace Sample
{
	struct Profiler
	{
		std::deque<SceneTimingData> m_LoadData;

		void AddData(const SceneTimingData& data)
		{
			m_LoadData.push_back(data);
		}

		void AddData(SceneTimingData&& data)
		{
			m_LoadData.push_back(std::move(data));
		}

		void Save(const wchar_t* filename)
		{
			assert(filename != nullptr);

			// Open file.
			int file = -1;

			(void)_wsopen_s(&file, filename, _O_CREAT | _O_TRUNC | _O_NOINHERIT | _O_BINARY | _O_SEQUENTIAL | _O_WRONLY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
			char* buffer = nullptr;

			[this,&file, &buffer]()
			{
				if (file == -1)
				{
					// can't open file for some reason.
					return;
				}

				auto headerString = "mapName,mapSize Compressed(MiB),mapSize Uncompressed (MiB),loadTime(ms),ioTime(ms),dataRate(MiB/s),amplified data Rate (MiB/s),meanFrameTimeBeforeLoading(us),meanFrameTimeDuringLoading(us),frameCountDuringLoading,useDirectStorage,usePlacedResources\x0d\x0a";
				auto bufSize = snprintf(nullptr, 0, "%s", headerString) + 1;
				buffer = static_cast<char*>(malloc(bufSize));

				if (!buffer) return;

				auto bufferStringLength = snprintf(buffer, bufSize, "%s", headerString);
				if (_write(file, buffer, bufferStringLength) != bufferStringLength)
				{
					return;
				}

				const char* formatString = "%s,%-01.2f,%-01.2f,%-01.2f,%-01.2f,%-01.2f,%-01.2f,%-01.2f,%-01.2f,%u,%u,%u\x0d\x0a";

				for (auto& data : m_LoadData)
				{
					auto bytesWrittenWithoutNullTerminator = snprintf(buffer, bufSize, formatString, data.loadRequest.m_sceneName->c_str(), data.sceneTextureFileDataSize/1024.0/1024.0, data.sceneTextureUncompressedSize/1024.0/1024.0, data.loadTime, data.ioTime, data.sceneTextureFileDataSize / data.ioTime / 1024.0, data.sceneTextureUncompressedSize / data.ioTime / 1024.0, data.frameTimeMeanBeforeLoading, data.frameTimes.GetArithmeticMean(), (unsigned)data.frameTimes.GetPopulationCount(), (unsigned)data.loadRequest.m_useDirectStorage, (unsigned)data.loadRequest.m_usePlacedResources);
					auto bytesRequiredWithNullTerminator = bytesWrittenWithoutNullTerminator + 1;
					if (bytesRequiredWithNullTerminator > bufSize) // is the buffer large enough to include null terminator?
					{
						bufSize = bytesRequiredWithNullTerminator;
						buffer = static_cast<char*>(realloc(buffer, bytesRequiredWithNullTerminator));
						if (!buffer) break; // premature termination of output because realloc returned nullptr.

						// retry with resized buffer.
						bytesWrittenWithoutNullTerminator = snprintf(buffer, bufSize, formatString, data.loadRequest.m_sceneName->c_str(), data.sceneTextureFileDataSize / 1024.0 / 1024.0, data.sceneTextureUncompressedSize / 1024.0 / 1024.0, data.loadTime, data.ioTime, data.sceneTextureFileDataSize / data.ioTime / 1024.0, data.sceneTextureUncompressedSize / data.ioTime / 1024.0, data.frameTimeMeanBeforeLoading, data.frameTimes.GetArithmeticMean(), (unsigned)data.frameTimes.GetPopulationCount(), (unsigned)data.loadRequest.m_useDirectStorage, (unsigned)data.loadRequest.m_usePlacedResources);
					}

					if (bytesWrittenWithoutNullTerminator > 0)
					{
						auto numBytesWritten = _write(file, buffer, bytesWrittenWithoutNullTerminator);
						if (numBytesWritten != bytesWrittenWithoutNullTerminator)
						{
							// something bad happened, and we should stop.
							return;
						}
					}
				}
			}();

			if (buffer)
			{
				free(buffer);
			}

			if (file != -1)
			{
				(void)_close(file);
			}


		}
	};
};
