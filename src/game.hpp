// This file is part of Playground3D.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <yttrium/base/clock.h>

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
	void update(const Yt::Window&);

private:
	Settings& _settings;
	Yt::Clock _clock;
	const std::unique_ptr<class GameState> _state;
	const std::unique_ptr<class WorldWidget> _world;
	const std::unique_ptr<class MinimapWidget> _leftMinimap;
	const std::unique_ptr<class MinimapWidget> _rightMinimap;
};
