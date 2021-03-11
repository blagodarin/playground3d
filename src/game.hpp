// This file is part of Playground3D.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <memory>
#include <optional>

namespace Yt
{
	class GuiFrame;
	class RenderPass;
	class ResourceLoader;
	class Window;
	class Vector2;
	class Vector3;
}

class Settings;

class Game
{
public:
	Game(Yt::ResourceLoader&, Settings&);
	~Game() noexcept;

	Yt::Vector3 cameraPosition() const noexcept;
	std::optional<Yt::Vector2> cursorCell() const noexcept;
	void mainScreen(Yt::GuiFrame&, Yt::RenderPass&);
	void update(const Yt::Window&, std::chrono::milliseconds);

private:
	Settings& _settings;
	const std::unique_ptr<class GameState> _state;
	const std::unique_ptr<class WorldCanvas> _world;
	const std::unique_ptr<class MinimapCanvas> _minimap;
};
