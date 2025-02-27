cmake_minimum_required(VERSION 3.22)

project(iglo LANGUAGES CXX)

# requires C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# iglo source files
set(IGLO_SOURCES
    ${CMAKE_SOURCE_DIR}/src/iglo.h
    ${CMAKE_SOURCE_DIR}/src/iglo.cpp
    ${CMAKE_SOURCE_DIR}/src/igloBatchRenderer.h
    ${CMAKE_SOURCE_DIR}/src/igloBatchRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/igloCamera.h
    ${CMAKE_SOURCE_DIR}/src/igloCamera.cpp
    ${CMAKE_SOURCE_DIR}/src/igloFont.h
    ${CMAKE_SOURCE_DIR}/src/igloFont.cpp
    ${CMAKE_SOURCE_DIR}/src/igloMainLoop.h
    ${CMAKE_SOURCE_DIR}/src/igloMainLoop.cpp
    ${CMAKE_SOURCE_DIR}/src/igloUtility.h
    ${CMAKE_SOURCE_DIR}/src/igloUtility.cpp
    ${CMAKE_SOURCE_DIR}/src/igloConfig.h
)

# set Agility SDK version
set(AGILITY_SDK_VERSION "1.715.1-preview")

# set download URL for Agility SDK
set(AGILITY_SDK_URL "https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/${AGILITY_SDK_VERSION}")

# set the path where the SDK will be downloaded and extracted
set(AGILITY_SDK_PACKAGE_DIR "${CMAKE_BINARY_DIR}/packages/Microsoft.Direct3D.D3D12.${AGILITY_SDK_VERSION}")
set(AGILITY_SDK_NUPKG_FILE "${CMAKE_BINARY_DIR}/Microsoft.Direct3D.D3D12.${AGILITY_SDK_VERSION}.nupkg")

# download the .nupkg file if it doesn't exist
if(NOT EXISTS "${AGILITY_SDK_NUPKG_FILE}")
    message(STATUS "Downloading Agility SDK from NuGet...")
    file(DOWNLOAD
        ${AGILITY_SDK_URL}
        ${AGILITY_SDK_NUPKG_FILE}
        TLS_VERIFY ON
        STATUS download_status
    )
    list(GET download_status 0 status_code)
    if(NOT status_code EQUAL 0)
        message(FATAL_ERROR "Failed to download Agility SDK. Check your internet connection.")
    endif()
    message(STATUS "Successfully downloaded Agility SDK.")
endif()

# extract the .nupkg file if it hasn't been extracted yet
if(NOT EXISTS "${AGILITY_SDK_PACKAGE_DIR}")
    message(STATUS "Extracting Agility SDK...")
    file(MAKE_DIRECTORY ${AGILITY_SDK_PACKAGE_DIR})
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${AGILITY_SDK_NUPKG_FILE}
        WORKING_DIRECTORY ${AGILITY_SDK_PACKAGE_DIR}
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Failed to extract Agility SDK.")
    endif()
    message(STATUS "Successfully extracted Agility SDK.")
endif()

# set path to the extracted SDK files
set(AGILITY_SDK_INCLUDE_DIR "${AGILITY_SDK_PACKAGE_DIR}/build/native/include")
set(AGILITY_SDK_DLL64_DIR "${AGILITY_SDK_PACKAGE_DIR}/build/native/bin/x64")

# Agility SDK files that will be copied to D3D12 folder
set(AGILITY_SDK_DLLS
    "${AGILITY_SDK_DLL64_DIR}/D3D12Core.dll"
    "${AGILITY_SDK_DLL64_DIR}/D3D12SDKLayers.dll"
    "${AGILITY_SDK_DLL64_DIR}/DirectSR.dll"
)

function(add_example name)
    set(example_fullpath ${CMAKE_CURRENT_SOURCE_DIR}/examples/${name})

    # first argument is the project name
    # remaining arguments are source file names relative to the "examples/<name>/" folder
    set(sources)
    foreach(file IN LISTS ARGN)
        list(APPEND sources ${example_fullpath}/${file})
    endforeach()

    if(MSVC)
        # set warning level
        add_compile_options(/W3 /WX)
    endif()

    add_executable(${name} ${sources} ${IGLO_SOURCES})

    if(MSVC)
        # set linker subsystem
        target_link_options(${name} PRIVATE "/SUBSYSTEM:WINDOWS")
        # no filter for example project source files
        source_group(" " FILES ${sources})
    endif()

    target_include_directories(${name} PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${AGILITY_SDK_INCLUDE_DIR}
    )

    # create 'iglo' filter
    source_group("iglo" FILES ${IGLO_SOURCES})
    
    # create D3D12 folder
    add_custom_command(
        TARGET ${name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${name}>/D3D12
    )
    # copy D3D12 files
    foreach(d3d12file ${AGILITY_SDK_DLLS})
        add_custom_command(
            TARGET ${name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${d3d12file} $<TARGET_FILE_DIR:${name}>/D3D12
        )
    endforeach()

    # copy resources
    if(EXISTS "${example_fullpath}/resources")
      add_custom_command(
          TARGET ${name} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_directory ${example_fullpath}/resources $<TARGET_FILE_DIR:${name}>/resources
      )
    endif()

    # copy shaders
    if(EXISTS "${example_fullpath}/shaders")
      add_custom_command(
          TARGET ${name} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_directory ${example_fullpath}/shaders $<TARGET_FILE_DIR:${name}>/shaders
      )
    endif()

    # set executable output directory
    # and set working dir to 'example-name/Debug' or 'example-name/Release' whichever is active.
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/${name}
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/examples/${name}/${CMAKE_CFG_INTDIR}
    )

endfunction()

add_example(hello-world
    HelloWorld.cpp
)
add_example(hello-batchrenderer
    HelloBatchRenderer.cpp
    AdvDrawStringFuncs.h
    AdvDrawStringFuncs.cpp
)
add_example(hello-cube
    HelloCube.cpp
)
add_example(camera-and-skybox
    CameraAndSkybox.cpp
    DemoCamera.h
    DemoCamera.cpp
    LoadingScreenProgressBar.h
    LoadingScreenProgressBar.cpp
)
add_example(lighting-and-shadows
    LightingAndShadows.cpp
)
