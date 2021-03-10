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
#include <yttrium/script/context.h>
#include <yttrium/storage/paths.h>
#include <yttrium/storage/storage.h>
#include <yttrium/storage/writer.h>
#include "game.hpp"

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
	Game game{ resourceLoader };
	window.show();
	for (Yt::RenderStatistics statistics; application.process_events(); statistics.add_frame())
	{
		Yt::GuiFrame gui{ guiState };
		if (gui.keyPressed(Yt::Key::F1))
			game.toggleDebugText();
		if (gui.keyPressed(Yt::Key::Escape))
			window.close();
		game.update(window, statistics.last_frame_duration());
		viewport.render(statistics.current_report(), [&](Yt::RenderPass& pass) {
			if (const auto cursor = gui.mouseArea(Yt::Rect{ window.size() }))
				game.setWorldCursor(Yt::Vector2{ *cursor });
			game.drawWorld(pass);
			game.drawMinimap(pass);
			{
				Yt::PushTexture push_texture{ pass, nullptr };
				pass.draw_rect(Yt::RectF{ Yt::Vector2{ window.cursor() }, Yt::SizeF{ 2, 2 } }, { 1, 1, 0, 1 });
			}
			game.drawDebugText(pass, statistics.previous_report());
		});
		if (gui.keyPressed(Yt::Key::F10))
			viewport.take_screenshot().save_as_screenshot(Yt::ImageFormat::Jpeg, 90);
	}
	return 0;
}
