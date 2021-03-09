// This file is part of Playground3D.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <yttrium/application/application.h>
#include <yttrium/application/key.h>
#include <yttrium/application/window.h>
#include <yttrium/gui/gui.h>
#include <yttrium/gui/ion_gui.h>
#include <yttrium/image/image.h>
#include <yttrium/image/utils.h>
#include <yttrium/logger.h>
#include <yttrium/main.h>
#include <yttrium/math/color.h>
#include <yttrium/renderer/report.h>
#include <yttrium/renderer/viewport.h>
#include <yttrium/resource_loader.h>
#include <yttrium/script/context.h>
#include <yttrium/storage/paths.h>
#include <yttrium/storage/storage.h>
#include <yttrium/storage/writer.h>
#include "game.hpp"

namespace
{
	void make_checkerboard_texture(Yt::Storage& storage, std::string_view name)
	{
		storage.attach_buffer(name, Yt::make_bgra32_tga(128, 128, [](size_t x, size_t y) {
			return ((x ^ y) & 1) ? Yt::Bgra32{ 0xdd, 0xdd, 0xdd } : Yt::Bgra32{ 0x00, 0x00, 0x00 };
		}));
	}
}

int ymain(int, char**)
{
	Yt::Logger logger;

	Yt::Storage storage{ Yt::Storage::UseFileSystem::Never };
	storage.attach_package("playground3d.ypq");
	::make_checkerboard_texture(storage, "data/checkerboard.tga");

	Yt::Application application;
	Yt::Window window{ application };
	Yt::Viewport viewport{ window };
	viewport.on_screenshot([](Yt::Image&& image) { image.save_as_screenshot(Yt::ImageFormat::Jpeg, 90); });

	Yt::ResourceLoader resource_loader{ storage, &viewport.render_manager() };

	Yt::GuiState gui_state;
	window.on_key_event([&gui_state](const Yt::KeyEvent& event) { gui_state.processKeyEvent(event); });

	Yt::ScriptContext script;
	Yt::IonGui ion_gui{ "data/gui.ion", resource_loader, script };
	Game game{ resource_loader, ion_gui };
	viewport.on_render([&ion_gui, &game](Yt::RenderPass& pass, const Yt::Vector2& cursor, const Yt::RenderReport& report) {
		ion_gui.draw(pass, cursor);
		game.draw_debug_graphics(pass, cursor, report);
	});

	ion_gui.start();
	window.set_title("Playground3D");
	window.show();
	for (Yt::RenderStatistics statistics; application.process_events(); statistics.add_frame())
	{
		Yt::GuiFrame gui{ gui_state };
		if (gui.keyPressed(Yt::Key::F1))
			game.toggle_debug_text();
		if (gui.keyPressed(Yt::Key::F10))
			viewport.take_screenshot();
		if (gui.keyPressed(Yt::Key::Escape))
			window.close();
		game.update(window, statistics.last_frame_duration());
		viewport.render(statistics.current_report(), statistics.previous_report());
	}
	return 0;
}
