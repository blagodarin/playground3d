// This file is part of Playground3D.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <memory>

namespace Yt
{
	class IonGui;
	class RenderPass;
	struct RenderReport;
	class ResourceLoader;
	class Window;
	class Vector2;
}

class GameState;
class MinimapCanvas;
class WorldCanvas;

class Game
{
public:
	Game(Yt::ResourceLoader&, Yt::IonGui&);
	~Game() noexcept;

	void draw_debug_graphics(Yt::RenderPass&, const Yt::Vector2& cursor, const Yt::RenderReport&);
	void toggle_debug_text() noexcept;
	void update(const Yt::Window&, std::chrono::milliseconds);

private:
	const std::unique_ptr<GameState> _state;
	const std::unique_ptr<WorldCanvas> _world;
	const std::unique_ptr<MinimapCanvas> _minimap;
};
