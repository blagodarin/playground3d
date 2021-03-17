// This file is part of Playground3D.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <yttrium/application/application.h>
#include <yttrium/application/key.h>
#include <yttrium/application/window.h>
#include <yttrium/gui/font.h>
#include <yttrium/gui/gui.h>
#include <yttrium/gui/layout.h>
#include <yttrium/image/image.h>
#include <yttrium/image/utils.h>
#include <yttrium/logger.h>
#include <yttrium/main.h>
#include <yttrium/math/color.h>
#include <yttrium/math/rect.h>
#include <yttrium/math/vector.h>
#include <yttrium/renderer/2d.h>
#include <yttrium/renderer/report.h>
#include <yttrium/renderer/viewport.h>
#include <yttrium/resource_loader.h>
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

		void present(Yt::GuiFrame& gui, const Yt::RenderReport& report, const Game& game, const Yt::Point& cursor)
		{
			if (gui.captureKeyDown(Yt::Key::F1))
				_showDebugText = !_showDebugText;
			gui.selectBlankTexture();
			gui.renderer().setColor(Yt::Bgra32::yellow());
			gui.renderer().addRect(Yt::RectF{ Yt::Vector2{ cursor }, Yt::SizeF{ 2, 2 } });
			if (_showDebugText)
			{
				gui.layout().fromTopLeft(Yt::GuiLayout::Axis::Y);
				gui.layout().setSize({ 0, 32 });
				gui.label(fmt::format("fps={},maxFrameTime={}ms", report._fps, report._max_frame_time.count()));
				gui.label(fmt::format("triangles={},drawCalls={}", report._triangles, report._draw_calls));
				gui.label(fmt::format("textureSwitches=(total={},redundant={})", report._texture_switches, report._extra_texture_switches));
				gui.label(fmt::format("shaderSwitches=(total={},redundant={})", report._shader_switches, report._extra_shader_switches));
				const auto camera = game.cameraPosition();
				gui.label(fmt::format("camera=(x={},y={},z={})", camera.x, camera.y, camera.z));
				if (const auto cell = game.cursorCell())
					gui.label(fmt::format("cell=(x={},y={})", static_cast<int>(cell->x), static_cast<int>(cell->y)));
				else
					gui.label("cell=()");
			}
		}

	private:
		Settings& _settings;
		bool _showDebugText = false;
	};
}

int ymain(int, char**)
{
	Yt::Logger logger;
	Yt::Storage storage{ Yt::Storage::UseFileSystem::Never };
	storage.attach_package("playground3d.ypq");
	storage.attach_buffer("data/checkerboard.tga", Yt::make_bgra32_tga(128, 128, [](size_t x, size_t y) {
		return ((x ^ y) & 1) ? Yt::Bgra32::grayscale(0xdd) : Yt::Bgra32::black();
	}));
	Yt::Application application;
	Yt::Window window{ application, "Playground3D" };
	Yt::Viewport viewport{ window };
	std::shared_ptr<const Yt::Font> font;
	if (const auto fontSource = storage.open("data/fonts/SourceCodePro-Regular.ttf"))
		font = Yt::Font::load(*fontSource, viewport.render_manager());
	Yt::GuiState guiState{ window };
	guiState.setDefaultFont(font);
	window.on_key_event([&guiState](const Yt::KeyEvent& event) { guiState.processKeyEvent(event); });
	Yt::Renderer2D rendered2d{ viewport };
	Yt::ResourceLoader resourceLoader{ storage, &viewport.render_manager() };
	Settings settings{ Yt::user_data_path("Playground3D") / "settings.ion" };
	Game game{ resourceLoader, settings };
	DebugGraphics debugGraphics{ settings };
	window.show();
	for (Yt::RenderClock clock; application.process_events(); clock.advance())
	{
		Yt::GuiFrame gui{ guiState, rendered2d };
		game.update(window, std::chrono::duration_cast<std::chrono::milliseconds>(clock.last_frame_duration()));
		viewport.render(clock.next_report(), [&](Yt::RenderPass& pass) {
			game.mainScreen(gui, pass);
			debugGraphics.present(gui, clock.last_report(), game, window.cursor());
			rendered2d.draw(pass);
		});
		if (gui.captureKeyDown(Yt::Key::F10))
			viewport.take_screenshot().save_as_screenshot(Yt::ImageFormat::Jpeg, 90);
		if (gui.captureKeyDown(Yt::Key::Escape))
			window.close();
	}
	return 0;
}
