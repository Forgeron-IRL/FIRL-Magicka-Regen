cmake_minimum_required(VERSION 3.21)

project(FIRLMagickaRegen VERSION 1.0.0 LANGUAGES CXX)

# CommonLibSSE-NG (alandtse, v15.5.0) as a submodule: the maintained fork, which
# reads the Address Library of Skyrim 1.7.x.
set(BUILD_TESTS OFF CACHE BOOL "No CommonLib tests" FORCE)
# No VR: CommonLib would link openvr_api.lib by absolute path, and an apostrophe in
# the project path breaks an MSBuild condition (MSB4100).
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "No VR support" FORCE)
add_subdirectory(extern/CommonLibSSE-NG CommonLibSSE EXCLUDE_FROM_ALL)
# When built from source, CommonLib does not include its plugin helper itself.
include(extern/CommonLibSSE-NG/cmake/CommonLibSSE.cmake)

find_path(SIMPLEINI_INCLUDE_DIRS "SimpleIni.h")

add_commonlibsse_plugin(${PROJECT_NAME}
    AUTHOR Forgeron-IRL
    SOURCES src/plugin.cpp
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

# Our minimal PCH (operator""sv at global scope) must be found by /FI.
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${SIMPLEINI_INCLUDE_DIRS}"
)

target_compile_options(${PROJECT_NAME} PRIVATE
    /permissive-
    /Zc:preprocessor
    /EHsc
    /utf-8
    /FIpch.h
)

# Post-build packaging.
# The zip mirrors the Data folder: SKSE/Plugins (the DLL) + the ESL and the
# MCM Helper files (MCM/Config/...) from assets/.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${CMAKE_SOURCE_DIR}/dist/SKSE/Plugins"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${PROJECT_NAME}>"
        "${CMAKE_SOURCE_DIR}/dist/SKSE/Plugins/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "${CMAKE_SOURCE_DIR}/dist"
    COMMAND ${CMAKE_COMMAND} -E chdir "${CMAKE_SOURCE_DIR}/dist"
        ${CMAKE_COMMAND} -E tar "cf"
        "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}-${PROJECT_VERSION}.zip"
        --format=zip "."
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_SOURCE_DIR}/dist"
    COMMENT "Packaging ${PROJECT_NAME}-${PROJECT_VERSION}.zip"
)
