include(${CMAKE_CURRENT_SOURCE_DIR}/../../common.cmake)

add_library(DirectStorageSample_Common STATIC DirectStorageSampleTexturePackageFormat.h PackageUtils.h PackageUtils.cpp CompressionSupport.h CompressionSupport.cpp)

target_link_libraries(DirectStorageSample_Common shlwapi Cauldron_DX12 DIRECTSTORAGE)
target_include_directories(DirectStorageSample_Common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

set(config
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common/DirectStorageSample.json
)

copyTargetCommand("${config}" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} copied_common_config)
add_dependencies(DirectStorageSample_Common copied_common_config)
