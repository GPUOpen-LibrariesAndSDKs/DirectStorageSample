cmake . -G "Visual Studio 17 2022" -A x64 -B buildx -DGFX_API=DX12
cmake --build buildx --config RelWithDebInfo