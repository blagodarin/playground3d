// This file is part of Playground3D.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "game.hpp"

#include <yttrium/application/key.h>
#include <yttrium/application/window.h>
#include <yttrium/gui/gui.h>
#include <yttrium/gui/layout.h>
#include <yttrium/math/line.h>
#include <yttrium/math/matrix.h>
#include <yttrium/math/quad.h>
#include <yttrium/math/rect.h>
#include <yttrium/renderer/2d.h>
#include <yttrium/renderer/modifiers.h>
#include <yttrium/renderer/pass.h>
#include <yttrium/utils/string.h>

#include "model.hpp"
#include "settings.hpp"

#include <fmt/format.h>

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
	Yt::Vector3 _position{ 0, -8.5, 16 };
	std::optional<Yt::Quad> _visible_area;
	std::optional<Yt::Vector2> _board_point;
	bool _showLeftMinimap = true;
	bool _showRightMinimap = true;
	bool _showInput = false;
	std::string _inputText = "Editable?";

	Yt::Matrix4 camera_matrix() const noexcept
	{
		return Yt::Matrix4::camera(_position, _rotation);
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

	void update_visible_area(Yt::RenderPass& pass)
	{
		Yt::Vector3 top_left;
		Yt::Vector3 top_right;
		Yt::Vector3 bottom_left;
		Yt::Vector3 bottom_right;
		const auto r = pass.viewport_rect();
		if (pass.pixel_ray(r.top_left()).plane_intersection(_board_plane, top_left)
			&& pass.pixel_ray(r.top_right()).plane_intersection(_board_plane, top_right)
			&& pass.pixel_ray(r.bottom_left()).plane_intersection(_board_plane, bottom_left)
			&& pass.pixel_ray(r.bottom_right()).plane_intersection(_board_plane, bottom_right))
			_visible_area = { { top_left.x, top_left.y }, { top_right.x, top_right.y }, { bottom_right.x, bottom_right.y }, { bottom_left.x, bottom_left.y } };
		else
			_visible_area.reset();
	}
};

class WorldWidget
{
public:
	WorldWidget(GameState& state, Yt::ResourceLoader& resource_loader)
		: _state{ state }
		, _cube{ resource_loader, "data/cube.obj", "data/cube.material" }
		, _checkerboard{ resource_loader, "data/checkerboard.obj", "data/checkerboard.material" }
	{
	}

	void present(Yt::GuiFrame& gui, Yt::RenderPass& pass)
	{
		_cursor = gui.hoverArea(pass.viewport_rect());
		Yt::Push3D projection{ pass, Yt::Matrix4::perspective(pass.viewport_rect().size(), 35, .5, 256), _state.camera_matrix() };
		_state.update_visible_area(pass);
		if (_cursor)
			_state.update_board_point(pass, *_cursor);
		else
			_state._board_point.reset();
		if (_state._board_point)
		{
			Yt::PushTransformation t{ pass, Yt::Matrix4::translation({ _state._board_point->x + .5f, _state._board_point->y + .5f, .5f }) };
			_cube.draw(pass);
		}
		_checkerboard.draw(pass);
	}

private:
	GameState& _state;
	Model _cube;
	Model _checkerboard;
	std::optional<Yt::Vector2> _cursor;
};

class MinimapWidget
{
public:
	MinimapWidget(GameState& state, std::string_view id)
		: _state{ state }, _id{ id } {}

	void present(Yt::GuiFrame& gui, const Yt::RectF& rect)
	{
		_cursor = gui.dragArea(_id, rect, Yt::Key::Mouse1);
		if (_cursor)
			_state.set_position(to_map(rect, *_cursor) - Yt::Vector2{ 0, 10 });
		gui.selectBlankTexture();
		gui.renderer().setColor(Yt::Bgra32::grayscale(64, 192));
		gui.renderer().addRect(rect);
		if (_state._visible_area)
		{
			gui.renderer().setColor(Yt::Bgra32::yellow(64));
			gui.renderer().addQuad(to_window(rect, *_state._visible_area));
		}
		if (_cursor)
		{
			gui.renderer().setColor(Yt::Bgra32::green());
			gui.renderer().addRect({ *_cursor, Yt::SizeF{ 1, 1 } });
		}
		gui.renderer().setColor(Yt::Bgra32::red());
		gui.renderer().addRect({ to_window(rect, { _state._position.x, _state._position.y }) - Yt::Vector2{ 2, 2 }, Yt::SizeF{ 4, 4 } });
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
	const std::string _id;
	std::optional<Yt::Vector2> _cursor;
};

Game::Game(Yt::ResourceLoader& resourceLoader, Settings& settings)
	: _settings{ settings }
	, _state{ std::make_unique<GameState>() }
	, _world{ std::make_unique<WorldWidget>(*_state, resourceLoader) }
	, _leftMinimap{ std::make_unique<MinimapWidget>(*_state, "LeftMinimap") }
	, _rightMinimap{ std::make_unique<MinimapWidget>(*_state, "RightMinimap") }
{
	if (const auto values = _settings.get("Camera"); values.size() == 2)
	{
		auto x = _state->_position.x;
		auto y = _state->_position.y;
		if (Yt::from_chars(values[0], x) && Yt::from_chars(values[1], y))
			_state->set_position({ x, y });
	}
}

Game::~Game() noexcept
{
	_settings.set("Camera", { fmt::format("{}", _state->_position.x), fmt::format("{}", _state->_position.y) });
}

Yt::Vector3 Game::cameraPosition() const noexcept
{
	return _state->_position;
}

std::optional<Yt::Vector2> Game::cursorCell() const noexcept
{
	return _state->_board_point;
}

void Game::mainScreen(Yt::GuiFrame& gui, Yt::RenderPass& pass)
{
	Yt::GuiLayout layout{ pass.viewport_rect() };
	layout.scaleForHeight(100);
	layout.setSpacing(1);
	layout.fromBottomLeft(Yt::GuiLayout::Axis::X, 1);
	const auto leftMinimapRect = layout.add({ 20, 20 });
	if (gui.button("ToggleLeftMinimap", _state->_showLeftMinimap ? "Hide" : "Show", layout.add({ 8, 3 })))
		_state->_showLeftMinimap = !_state->_showLeftMinimap;
	if (_state->_showLeftMinimap)
		_leftMinimap->present(gui, leftMinimapRect);
	layout.fromBottomRight(Yt::GuiLayout::Axis::X, 1);
	const auto rightMinimapRect = layout.add({ 20, 20 });
	if (gui.button("ToggleRightMinimap", _state->_showRightMinimap ? "Hide" : "Show", layout.add({ 8, 3 })))
		_state->_showRightMinimap = !_state->_showRightMinimap;
	if (_state->_showRightMinimap)
		_rightMinimap->present(gui, rightMinimapRect);
	layout.fromTopRight(Yt::GuiLayout::Axis::Y, 1);
	if (gui.button("ToggleInput", _state->_showInput ? "Hide input" : "Show input", layout.add({ 20, 3 })))
		_state->_showInput = !_state->_showInput;
	if (_state->_showInput)
		gui.stringEdit("Input", _state->_inputText, layout.add({ 20, 3 }));
	_world->present(gui, pass);
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
