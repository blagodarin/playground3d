# This file is part of Playground3D.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.19)

project(Playground3D LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(YTTRIUM_WITH_JPEG ON) # For screenshots.
set(YTTRIUM_WITH_OGGVORBIS OFF)

include(FetchContent)
FetchContent_Declare(Yttrium GIT_REPOSITORY https://github.com/blagodarin/yttrium.git GIT_TAG 92b92598efb8c4526cc8b0298543c4ac2e646e9f)
FetchContent_MakeAvailable(Yttrium)

add_executable(playground3d WIN32
	src/game.cpp
	src/game.hpp
	src/main.cpp
	src/model.cpp
	src/model.hpp
	src/settings.cpp
	src/settings.hpp
	)
target_link_libraries(playground3d PRIVATE yapplication yengine ygui ymain)
set_target_properties(playground3d PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

yttrium_target_package(playground3d
	OUTPUT playground3d.ypq
	INDEX data/index.txt)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT playground3d)
