// This file is part of Playground3D.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <memory>

namespace Yt
{
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
	Game(Yt::ResourceLoader&);
	~Game() noexcept;

	void drawDebugText(Yt::RenderPass&, const Yt::RenderReport&);
	void drawMinimap(Yt::RenderPass&);
	void drawWorld(Yt::RenderPass&);
	void setWorldCursor(const Yt::Vector2&);
	void toggleDebugText() noexcept;
	void update(const Yt::Window&, std::chrono::milliseconds);

private:
	const std::unique_ptr<GameState> _state;
	const std::unique_ptr<WorldCanvas> _world;
	const std::unique_ptr<MinimapCanvas> _minimap;
};
