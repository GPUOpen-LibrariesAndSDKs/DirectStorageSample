add_library(DIRECTSTORAGE SHARED IMPORTED GLOBAL)

set(DStorageHashExpected 6e8d68fdb2d28fa48f1b56a44069ff46)
file(DOWNLOAD https://globalcdn.nuget.org/packages/microsoft.direct3d.directstorage.1.2.1.nupkg ${CMAKE_CURRENT_SOURCE_DIR}/dstorage.nupkg EXPECTED_HASH MD5=${DStorageHashExpected})
file(ARCHIVE_EXTRACT INPUT ${CMAKE_CURRENT_SOURCE_DIR}/dstorage.nupkg DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/.)


set_property(TARGET DIRECTSTORAGE PROPERTY IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/native/lib/x64/dstorage.lib)
set_property(TARGET DIRECTSTORAGE PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/native/bin/x64/dstorage.dll ${CMAKE_CURRENT_SOURCE_DIR}/native/bin/x64/dstoragecore.dll)
target_include_directories(DIRECTSTORAGE SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/native/include)