include(../../common.cmake)
set(sources
    ArtificialWorkload.h
    ArtificialWorkload.cpp
    DirectStorageSample.cpp
    DirectStorageSample.h
    GLTFTextureAndBuffersDirectStorage.h
    GLTFTextureAndBuffersDirectStorage.cpp
    Renderer.cpp
    Renderer.h
    UI.cpp
    UI.h
    Profile.h
    stdafx.cpp
    stdafx.h
    TransparentCube.h
    TransparentCube.cpp
    SampleOptions.h
    dpiawarescaling.manifest)


source_group("Sources" FILES ${sources})
source_group("Icon"    FILES ${icon_src}) # defined in top-level CMakeLists.txt


add_executable(DirectStorageSample_DX12 WIN32 ${sources} ${common} ${icon_src})
target_link_libraries(DirectStorageSample_DX12 LINK_PUBLIC DirectStorageSample_Common Cauldron_DX12 ImGUI amd_ags d3dcompiler D3D12 DIRECTSTORAGE)
set_target_properties(DirectStorageSample_DX12 PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/bin" DEBUG_POSTFIX "d")
add_custom_command(TARGET DirectStorageSample_DX12 POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:DirectStorageSample_DX12> $<TARGET_FILE_DIR:DirectStorageSample_DX12>
  COMMAND_EXPAND_LISTS
  )

set(Shaders_Sample_src
     ${CMAKE_CURRENT_SOURCE_DIR}/Mandelbrot.hlsl
     ${CMAKE_CURRENT_SOURCE_DIR}/TransparentCube.hlsl)

copyTargetCommand("${Shaders_Sample_src}" ${CMAKE_HOME_DIRECTORY}/bin/ShaderLibDX copied_dx12_shaders_sample)
set_source_files_properties(${Shaders_Sample_src} PROPERTIES VS_TOOL_OVERRIDE "Text")
add_dependencies(DirectStorageSample_DX12 copied_dx12_shaders_sample)
