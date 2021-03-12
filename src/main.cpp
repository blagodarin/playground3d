// This file is part of Playground3D.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <yttrium/application/application.h>
#include <yttrium/application/key.h>
#include <yttrium/application/window.h>
#include <yttrium/gui/gui.h>
#include <yttrium/image/image.h>
#include <yttrium/image/utils.h>
#include <yttrium/logger.h>
#include <yttrium/main.h>
#include <yttrium/math/color.h>
#include <yttrium/math/rect.h>
#include <yttrium/math/vector.h>
#include <yttrium/renderer/modifiers.h>
#include <yttrium/renderer/pass.h>
#include <yttrium/renderer/report.h>
#include <yttrium/renderer/viewport.h>
#include <yttrium/resource_loader.h>
#include <yttrium/storage/paths.h>
#include <yttrium/storage/storage.h>

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
			if (const auto values = _settings.get("Debug"); !values.empty() && values[0] == "1")
				_showText = true;
		}

		~DebugGraphics()
		{
			_settings.set("Debug", { _showText ? "1" : "0" });
		}

		void present(Yt::GuiFrame& gui, Yt::RenderPass& pass, const Yt::RenderReport& report, const Game& game, const Yt::Point& cursor)
		{
			if (gui.captureKeyDown(Yt::Key::F1))
				_showText = !_showText;
			{
				Yt::PushTexture push_texture{ pass, nullptr };
				pass.draw_rect(Yt::RectF{ Yt::Vector2{ cursor }, Yt::SizeF{ 2, 2 } }, { 1, 1, 0, 1 });
			}
			if (_showText)
			{
				const auto camera = game.cameraPosition();
				std::string debug_text;
				Yt::append_to(debug_text,
					"FPS: ", report._fps, '\n',
					"MaxFrameTime: ", report._max_frame_time.count(), " ms\n",
					"Triangles: ", report._triangles, '\n',
					"DrawCalls: ", report._draw_calls, '\n',
					"TextureSwitches: ", report._texture_switches, " (Redundant: ", report._extra_texture_switches, ")\n",
					"ShaderSwitches: ", report._shader_switches, " (Redundant: ", report._extra_shader_switches, ")\n",
					"Camera: X=", camera.x, ", Y=", camera.y, ", Z=", camera.z, '\n');
				if (const auto cell = game.cursorCell())
					Yt::append_to(debug_text, "Cell: (", static_cast<int>(cell->x), ",", static_cast<int>(cell->y), ")");
				else
					Yt::append_to(debug_text, "Cell: none");
				pass.add_debug_text(debug_text);
			}
		}

	private:
		Settings& _settings;
		bool _showText = false;
	};
}

int ymain(int, char**)
{
	Yt::Logger logger;
	Yt::Storage storage{ Yt::Storage::UseFileSystem::Never };
	storage.attach_package("playground3d.ypq");
	storage.attach_buffer("data/checkerboard.tga", Yt::make_bgra32_tga(128, 128, [](size_t x, size_t y) {
		return ((x ^ y) & 1) ? Yt::Bgra32{ 0xdd, 0xdd, 0xdd } : Yt::Bgra32{ 0x00, 0x00, 0x00 };
	}));
	Yt::Application application;
	Yt::Window window{ application, "Playground3D" };
	Yt::GuiState guiState{ window };
	window.on_key_event([&guiState](const Yt::KeyEvent& event) { guiState.processKeyEvent(event); });
	Yt::Viewport viewport{ window };
	Yt::ResourceLoader resourceLoader{ storage, &viewport.render_manager() };
	Settings settings{ Yt::user_data_path("Playground3D") / "settings.ion" };
	Game game{ resourceLoader, settings };
	DebugGraphics debugGraphics{ settings };
	window.show();
	for (Yt::RenderStatistics statistics; application.process_events(); statistics.add_frame())
	{
		Yt::GuiFrame gui{ guiState };
		game.update(window, statistics.last_frame_duration());
		viewport.render(statistics.current_report(), [&](Yt::RenderPass& pass) {
			game.mainScreen(gui, pass);
			debugGraphics.present(gui, pass, statistics.previous_report(), game, window.cursor());
		});
		if (gui.captureKeyDown(Yt::Key::F10))
			viewport.take_screenshot().save_as_screenshot(Yt::ImageFormat::Jpeg, 90);
		if (gui.captureKeyDown(Yt::Key::Escape))
			window.close();
	}
	return 0;
}
