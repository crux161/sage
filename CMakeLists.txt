cmake_minimum_required(VERSION 3.16)
project(SAGE LANGUAGES C CXX)

add_library(sage_ecs src/sage_ecs.c)
target_include_directories(sage_ecs
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_features(sage_ecs PUBLIC c_std_99)
set_target_properties(sage_ecs PROPERTIES C_EXTENSIONS OFF)

add_library(eshi_core STATIC
    vendor/eshi/core/src/abi.cpp
    vendor/eshi/core/src/world.cpp
    vendor/eshi/core/src/scene.cpp
    vendor/eshi/core/src/render/registry.cpp
    vendor/eshi/core/src/render/transpile.cpp
    vendor/eshi/core/src/render/ink.cpp
)
target_include_directories(eshi_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/eshi/core/include
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/libsumi/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/eshi/core/src
)
target_compile_features(eshi_core PUBLIC cxx_std_11)
set_target_properties(eshi_core PROPERTIES CXX_EXTENSIONS OFF)

add_library(sage_renderer STATIC src/sage_renderer.cpp)
target_include_directories(sage_renderer
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(sage_renderer PUBLIC cxx_std_11)
target_link_libraries(sage_renderer PUBLIC sage_ecs PRIVATE eshi_core)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/bullet3/VERSION" BULLET_VERSION)
set(BULLET_PHYSICS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/bullet3")
set(INSTALL_LIBS OFF)
add_subdirectory(vendor/bullet3/src/LinearMath)
add_subdirectory(vendor/bullet3/src/BulletCollision)
add_subdirectory(vendor/bullet3/src/BulletDynamics)

add_library(sage_phys STATIC src/sage_phys.cpp)
target_include_directories(sage_phys
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/bullet3/src
)
target_compile_features(sage_phys PUBLIC cxx_std_11)
target_link_libraries(sage_phys PUBLIC sage_ecs PRIVATE
    BulletDynamics BulletCollision LinearMath)

add_library(sage_audio STATIC src/sage_audio.c)
target_include_directories(sage_audio
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(sage_audio PUBLIC c_std_99)
target_link_libraries(sage_audio PUBLIC sage_ecs)
set_target_properties(sage_audio PROPERTIES C_EXTENSIONS OFF)

option(SAGE_BUILD_SDL3_DEMO "Build the SDL3 reference host" OFF)
option(SAGE_BUILD_SDL3_AUDIO "Build the SDL3 audio device adapter" OFF)
if(SAGE_BUILD_SDL3_DEMO OR SAGE_BUILD_SDL3_AUDIO)
    find_package(SDL3 CONFIG REQUIRED)
endif()

if(SAGE_BUILD_SDL3_AUDIO OR SAGE_BUILD_SDL3_DEMO)
    add_library(sage_audio_sdl3 STATIC src/sage_audio_sdl3.c)
    target_include_directories(sage_audio_sdl3
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_compile_features(sage_audio_sdl3 PUBLIC c_std_99)
    set_target_properties(sage_audio_sdl3 PROPERTIES C_EXTENSIONS OFF)
    target_link_libraries(sage_audio_sdl3 PUBLIC sage_audio SDL3::SDL3)
endif()

if(SAGE_BUILD_SDL3_DEMO)
    add_executable(sage_sdl3_demo examples/sdl3_demo.cpp)
    target_compile_features(sage_sdl3_demo PRIVATE cxx_std_11)
    target_link_libraries(sage_sdl3_demo PRIVATE
        sage_renderer sage_phys sage_audio_sdl3 SDL3::SDL3)
endif()
