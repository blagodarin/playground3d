# This file is part of Playground3D.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.19)

project(Playground3D LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(YTTRIUM_IMAGE_JPEG ON) # For screenshots.
set(YTTRIUM_IMAGE_TGA ON) # For generated images.

include(FetchContent)
FetchContent_Declare(Yttrium GIT_REPOSITORY https://github.com/blagodarin/yttrium.git GIT_TAG 7532eb4862956b8c78a6ec260554c39cc5108850)
FetchContent_MakeAvailable(Yttrium)

find_package(fmt REQUIRED) # For debug text.

add_executable(playground3d WIN32
	src/game.cpp
	src/game.hpp
	src/main.cpp
	src/model.cpp
	src/model.hpp
	src/settings.cpp
	src/settings.hpp
	)
target_link_libraries(playground3d PRIVATE Y_application Y_gui Y_image Y_ion Y_main Y_renderer Y_storage fmt::fmt)
set_target_properties(playground3d PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

yttrium_target_package(playground3d
	OUTPUT playground3d.yp
	INDEX data/index.txt)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT playground3d)
