// This file is part of Playground3D.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "game.hpp"

#include <yttrium/application/key.h>
#include <yttrium/application/window.h>
#include <yttrium/exceptions.h>
#include <yttrium/gui/gui.h>
#include <yttrium/ion/reader.h>
#include <yttrium/ion/writer.h>
#include <yttrium/logger.h>
#include <yttrium/math/line.h>
#include <yttrium/math/matrix.h>
#include <yttrium/math/quad.h>
#include <yttrium/math/rect.h>
#include <yttrium/renderer/report.h>
#include <yttrium/renderer/modifiers.h>
#include <yttrium/renderer/pass.h>
#include <yttrium/storage/paths.h>
#include <yttrium/storage/source.h>
#include <yttrium/storage/writer.h>
#include <yttrium/utils/string.h>
#include "model.hpp"

#include <optional>

namespace
{
	const Yt::Plane _board_plane{ { 0, 0, 1 }, { 0, 0, 0 } };
	const Yt::Euler _rotation{ 0, -60, 0 };

	Yt::Vector3 clamp_position(const Yt::Vector3& v)
	{
		return {
			std::clamp(v.x, -64.f + 12.75f, 64.f - 12.75f),
			std::clamp(v.y, -64.f - 3.5f, 64.f - 17.5f),
			std::clamp(v.z, 1.f, 64.f),
		};
	}
}

class GameState
{
public:
	bool _debug_text_visible = false;
	Yt::Vector3 _position{ 0, -8.5, 16 };
	std::optional<Yt::Quad> _visible_area;
	std::optional<Yt::Vector2> _board_point;

	Yt::Matrix4 camera_matrix() const noexcept
	{
		return Yt::Matrix4::camera(_position, _rotation);
	}

	void load(Yt::Source& source)
	{
		auto debug = false;
		auto x = _position.x;
		auto y = _position.y;
		try
		{
			Yt::IonReader ion{ source };
			for (auto token = ion.read(); token.type() != Yt::IonToken::Type::End; token = ion.read())
				if (const auto name = token.to_name(); name == "X")
					Yt::from_chars(ion.read().to_value(), x);
				else if (name == "Y")
					Yt::from_chars(ion.read().to_value(), y);
				else if (name == "Debug")
					debug = true;
		}
		catch (const Yt::IonError& e)
		{
			Yt::Logger::log("Bad settings: ", e.what());
			return;
		}
		_debug_text_visible = debug;
		set_position({ x, y });
	}

	void save(Yt::Writer&& writer) const
	{
		Yt::IonWriter ion{ writer, Yt::IonWriter::Formatting::Pretty };
		if (_debug_text_visible)
			ion.add_name("Debug");
		ion.add_name("X");
		ion.add_value(Yt::make_string(_position.x));
		ion.add_name("Y");
		ion.add_value(Yt::make_string(_position.y));
		ion.flush();
	}

	void set_position(const Yt::Vector2& position)
	{
		_position = ::clamp_position({ position, _position.z });
	}

	void update_board_point(Yt::RenderPass& pass, const Yt::Vector2& cursor)
	{
		const auto cursor_ray = pass.pixel_ray(cursor);
		if (Yt::Vector3 p; cursor_ray.plane_intersection(_board_plane, p) && std::abs(p.x) <= 64 && std::abs(p.y) <= 64)
			_board_point.emplace(std::floor(p.x), std::floor(p.y));
		else
			_board_point.reset();
	}

	void update_debug_text(Yt::RenderPass& pass, const Yt::RenderReport& report) const
	{
		if (!_debug_text_visible)
			return;
		std::string debug_text;
		Yt::append_to(debug_text,
			"FPS: ", report._fps, '\n',
			"MaxFrameTime: ", report._max_frame_time.count(), " ms\n",
			"Triangles: ", report._triangles, '\n',
			"DrawCalls: ", report._draw_calls, '\n',
			"TextureSwitches: ", report._texture_switches, " (Redundant: ", report._extra_texture_switches, ")\n",
			"ShaderSwitches: ", report._shader_switches, " (Redundant: ", report._extra_shader_switches, ")\n",
			"X: ", _position.x, ", Y: ", _position.y, ", Z: ", _position.z, '\n',
			"Cell: (");
		if (_board_point)
			Yt::append_to(debug_text, static_cast<int>(_board_point->x), ",", static_cast<int>(_board_point->y));
		else
			Yt::append_to(debug_text, "none");
		Yt::append_to(debug_text, ")");
		pass.add_debug_text(debug_text);
	}

	void update_visible_area(Yt::RenderPass& pass)
	{
		Yt::Vector3 top_left;
		Yt::Vector3 top_right;
		Yt::Vector3 bottom_left;
		Yt::Vector3 bottom_right;
		const Yt::RectF r{ Yt::SizeF{ pass.window_size() } };
		if (pass.pixel_ray(r.top_left()).plane_intersection(_board_plane, top_left)
			&& pass.pixel_ray(r.top_right()).plane_intersection(_board_plane, top_right)
			&& pass.pixel_ray(r.bottom_left()).plane_intersection(_board_plane, bottom_left)
			&& pass.pixel_ray(r.bottom_right()).plane_intersection(_board_plane, bottom_right))
			_visible_area = { { top_left.x, top_left.y }, { top_right.x, top_right.y }, { bottom_right.x, bottom_right.y }, { bottom_left.x, bottom_left.y } };
		else
			_visible_area.reset();
	}
};

class WorldCanvas final : public Yt::Canvas
{
public:
	explicit WorldCanvas(GameState& state, Yt::ResourceLoader& resource_loader)
		: _state{ state }
		, _cube{ resource_loader, "data/cube.obj", "data/cube.material" }
		, _checkerboard{ resource_loader, "data/checkerboard.obj", "data/checkerboard.material" }
	{
	}

	void on_draw(Yt::RenderPass& pass, const Yt::RectF&, std::chrono::milliseconds) override // TODO: Use the provided rect.
	{
		Yt::Push3D projection{ pass, Yt::Matrix4::perspective(pass.window_size(), 35, .5, 256), _state.camera_matrix() };
		_state.update_visible_area(pass);
		if (_cursor)
			_state.update_board_point(pass, *_cursor);
		if (_state._board_point)
		{
			Yt::PushTransformation t{ pass, Yt::Matrix4::translation({ _state._board_point->x + .5f, _state._board_point->y + .5f, .5f }) };
			_cube.draw(pass);
		}
		_checkerboard.draw(pass);
	}

	void on_mouse_move(const Yt::RectF&, const Yt::Vector2& cursor) override
	{
		_cursor = cursor;
	}

private:
	GameState& _state;
	Model _cube;
	Model _checkerboard;
	std::optional<Yt::Vector2> _cursor;
};

class MinimapCanvas final : public Yt::Canvas
{
public:
	explicit MinimapCanvas(GameState& state)
		: _state{ state } {}

	void on_draw(Yt::RenderPass& pass, const Yt::RectF& rect, std::chrono::milliseconds) override
	{
		Yt::PushTexture push_texture{ pass, nullptr };
		pass.draw_rect(rect, { 0.25, 0.25, 0.25, 0.75 });
		if (_state._visible_area)
			pass.draw_quad(to_window(rect, *_state._visible_area), { 1, 1, 0, 0.25 });
		if (_cursor)
			pass.draw_rect({ *_cursor, Yt::SizeF{ 1, 1 } }, { 0, 1, 0 });
		pass.draw_rect({ to_window(rect, { _state._position.x, _state._position.y }) - Yt::Vector2{ 2, 2 }, Yt::SizeF{ 4, 4 } }, { 1, 0, 0 });
	}

	void on_mouse_move(const Yt::RectF& rect, const Yt::Vector2& cursor) override
	{
		_state.set_position(to_map(rect, cursor) - Yt::Vector2{ 0, 10 });
		_cursor = cursor;
	}

	bool on_mouse_press(const Yt::RectF& rect, Yt::Key key, const Yt::Vector2& cursor) override
	{
		if (key != Yt::Key::Mouse1)
			return false;
		on_mouse_move(rect, cursor);
		return true;
	}

private:
	static Yt::Vector2 to_map(const Yt::RectF& rect, const Yt::Vector2& v)
	{
		return {
			(v.x - rect.left()) / rect.width() * 128 - 64,
			(rect.top() - v.y) / rect.height() * 128 + 64,
		};
	}

	static Yt::Vector2 to_window(const Yt::RectF& rect, const Yt::Vector2& v)
	{
		return rect.top_left() + Yt::Vector2{ rect.width(), rect.height() } * Yt::Vector2{ v.x + 64, 64 - v.y } / 128;
	}

	static Yt::Quad to_window(const Yt::RectF& rect, const Yt::Quad& q)
	{
		return { to_window(rect, q._a), to_window(rect, q._b), to_window(rect, q._c), to_window(rect, q._d) };
	}

private:
	GameState& _state;
	std::optional<Yt::Vector2> _cursor;
};

Game::Game(Yt::ResourceLoader& resource_loader, Yt::Gui& gui)
	: _state{ std::make_unique<GameState>() }
	, _world{ std::make_unique<WorldCanvas>(*_state, resource_loader) }
	, _minimap{ std::make_unique<MinimapCanvas>(*_state) }
{
	gui.bind_canvas("world", *_world);
	gui.bind_canvas("minimap", *_minimap);
	if (const auto source = Yt::Source::from(Yt::user_data_path("Playground3D") / "save.ion"))
		_state->load(*source);
}

Game::~Game() noexcept
{
	_state->save(Yt::Writer{ Yt::user_data_path("Playground3D") / "save.ion" });
}

void Game::draw_debug_graphics(Yt::RenderPass& pass, const Yt::Vector2& cursor, const Yt::RenderReport& report)
{
	{
		Yt::PushTexture push_texture{ pass, nullptr };
		pass.draw_rect(Yt::RectF{ cursor, Yt::SizeF{ 2, 2 } }, { 1, 1, 0, 1 });
	}
	_state->update_debug_text(pass, report);
}

void Game::toggle_debug_text() noexcept
{
	_state->_debug_text_visible = !_state->_debug_text_visible;
}

void Game::update(const Yt::Window& window, std::chrono::milliseconds advance)
{
	const bool move_forward = window.cursor()._y < 10;
	const bool move_backward = window.size()._height - window.cursor()._y <= 10;
	const bool move_left = window.cursor()._x < 10;
	const bool move_right = window.size()._width - window.cursor()._x <= 10;
	if (move_forward != move_backward || move_left != move_right)
	{
		constexpr auto speed = 16.f; // Units per second.
		const auto distance = static_cast<float>(advance.count()) * speed / 1000;
		const auto offset = (move_forward || move_backward) && (move_left || move_right) ? distance / std::numbers::sqrt2_v<float> : distance;
		const auto x_movement = move_left ? -offset : (move_right ? offset : 0);
		const auto y_movement = move_forward ? offset : (move_backward ? -offset : 0);
		_state->set_position({ _state->_position.x + x_movement, _state->_position.y + y_movement });
	}
}
