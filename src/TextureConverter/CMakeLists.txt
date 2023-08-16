set(sources
    TextureConverter.cpp
    stdafx.h)

source_group("Sources" FILES ${sources})
source_group("Icon"    FILES ${icon_src}) # defined in top-level CMakeLists.txt

add_executable(TextureConverter WIN32 ${sources} ${common} ${icon_src})
target_link_libraries(TextureConverter LINK_PUBLIC DirectStorageSample_Common Cauldron_DX12 D3D12 shlwapi DIRECTSTORAGE)

set_target_properties(TextureConverter PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/bin" DEBUG_POSTFIX "d" WIN32_EXECUTABLE FALSE)