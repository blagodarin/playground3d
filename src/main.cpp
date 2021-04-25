// This file is part of Playground3D.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <yttrium/application/application.h>
#include <yttrium/application/key.h>
#include <yttrium/application/window.h>
#include <yttrium/base/clock.h>
#include <yttrium/base/logger.h>
#include <yttrium/geometry/rect.h>
#include <yttrium/geometry/vector.h>
#include <yttrium/gui/context.h>
#include <yttrium/gui/font.h>
#include <yttrium/gui/gui.h>
#include <yttrium/gui/layout.h>
#include <yttrium/image/image.h>
#include <yttrium/main.h>
#include <yttrium/renderer/2d.h>
#include <yttrium/renderer/metrics.h>
#include <yttrium/renderer/resource_loader.h>
#include <yttrium/renderer/viewport.h>
#include <yttrium/storage/paths.h>
#include <yttrium/storage/source.h>
#include <yttrium/storage/storage.h>

#include <fmt/format.h>

#include "game.hpp"
#include "settings.hpp"

namespace
{
	class DebugGraphics
	{
	public:
		DebugGraphics(Settings& settings)
			: _settings{ settings }
		{
			if (const auto values = _settings.get("DebugText"); !values.empty() && values[0] == "1")
				_showDebugText = true;
		}

		~DebugGraphics()
		{
			_settings.set("DebugText", { _showDebugText ? "1" : "0" });
		}

		void present(Yt::GuiFrame& gui, const Game& game, const Yt::Point& cursor)
		{
			if (gui.takeKeyPress(Yt::Key::F1))
				_showDebugText = !_showDebugText;
			gui.selectBlankTexture();
			gui.renderer().setColor(Yt::Bgra32::yellow());
			gui.renderer().addRect(Yt::RectF{ Yt::Vector2{ cursor }, Yt::SizeF{ 2, 2 } });
			if (_showDebugText)
			{
				Yt::GuiLayout layout{ gui };
				layout.fromTopLeft(Yt::GuiLayout::Axis::Y);
				layout.setSize({ 0, 32 });
				gui.addLabel(fmt::format("fps={},maxFrameTime={}ms", _clockReport._framesPerSecond, _clockReport._maxFrameTime));
				gui.addLabel(fmt::format("triangles={},drawCalls={}", _metrics._triangles, _metrics._draw_calls));
				gui.addLabel(fmt::format("textureSwitches=(total={},redundant={})", _metrics._texture_switches, _metrics._extra_texture_switches));
				gui.addLabel(fmt::format("shaderSwitches=(total={},redundant={})", _metrics._shader_switches, _metrics._extra_shader_switches));
				const auto camera = game.cameraPosition();
				gui.addLabel(fmt::format("camera=(x={},y={},z={})", camera.x, camera.y, camera.z));
				if (const auto cell = game.cursorCell())
					gui.addLabel(fmt::format("cell=(x={},y={})", static_cast<int>(cell->x), static_cast<int>(cell->y)));
				else
					gui.addLabel("cell=()");
			}
		}

		void update(const Yt::RenderMetrics& metrics)
		{
			_nextMetrics += metrics;
			if (_clock.update(_clockReport))
			{
				_metrics = _nextMetrics / _clockReport._frameCount;
				_nextMetrics = {};
			}
		}

	private:
		Settings& _settings;
		bool _showDebugText = false;
		Yt::FrameClock _clock;
		Yt::FrameClock::Report _clockReport;
		Yt::RenderMetrics _metrics;
		Yt::RenderMetrics _nextMetrics;
	};
}

int ymain(int, char**)
{
	Yt::Logger logger;
	Yt::Storage storage{ Yt::Storage::UseFileSystem::Never };
	storage.attach_package(Yt::Source::from("playground3d.yp"));
	storage.attach_buffer("data/checkerboard.tga", Yt::Image::generateBgra32(128, 128, [](size_t x, size_t y) {
		return ((x ^ y) & 1) ? Yt::Bgra32::grayscale(0xdd) : Yt::Bgra32::black();
	}).to_buffer(Yt::ImageFormat::Tga));
	Yt::Application application;
	Yt::Window window{ application, "Playground3D" };
	Yt::Viewport viewport{ window };
	Yt::GuiContext gui{ window };
	if (const auto fontSource = storage.open("data/fonts/SourceCodePro-Regular.ttf"))
		gui.setDefaultFont(Yt::Font::load(*fontSource, viewport.render_manager()));
	Yt::Renderer2D rendered2d{ viewport };
	Yt::ResourceLoader resourceLoader{ storage, viewport.render_manager() };
	Settings settings{ Yt::user_data_path("Playground3D") / "settings.ion" };
	Game game{ resourceLoader, settings };
	DebugGraphics debugGraphics{ settings };
	window.show();
	while (application.process_events(gui.eventCallbacks()))
	{
		debugGraphics.update(viewport.metrics());
		Yt::GuiFrame guiFrame{ gui, rendered2d };
		game.update(window);
		viewport.render([&](Yt::RenderPass& pass) {
			game.mainScreen(guiFrame, pass);
			debugGraphics.present(guiFrame, game, window.cursor());
			rendered2d.draw(pass);
		});
		if (guiFrame.takeKeyPress(Yt::Key::F10))
			viewport.take_screenshot().save_as_screenshot(Yt::ImageFormat::Jpeg, 90);
		if (guiFrame.takeKeyPress(Yt::Key::Escape))
			window.close();
	}
	return 0;
}
