set(sources
    Timestamp.cpp)

source_group("Sources" FILES ${sources})
source_group("Icon"    FILES ${icon_src}) # defined in top-level CMakeLists.txt

add_executable(Timestamp WIN32 ${sources} ${common} ${icon_src})
target_link_libraries(Timestamp)

set_target_properties(Timestamp PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/bin" DEBUG_POSTFIX "d" WIN32_EXECUTABLE FALSE)