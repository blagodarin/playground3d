// This file is part of Playground3D.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "game.hpp"

#include <yttrium/application/key.h>
#include <yttrium/application/window.h>
#include <yttrium/gui/gui.h>
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
	Yt::Vector3 _position{ 0, -8.5, 16 };
	std::optional<Yt::Quad> _visible_area;
	std::optional<Yt::Vector2> _board_point;

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

class WorldCanvas
{
public:
	WorldCanvas(GameState& state, Yt::ResourceLoader& resource_loader)
		: _state{ state }
		, _cube{ resource_loader, "data/cube.obj", "data/cube.material" }
		, _checkerboard{ resource_loader, "data/checkerboard.obj", "data/checkerboard.material" }
	{
	}

	void draw(Yt::RenderPass& pass)
	{
		Yt::Push3D projection{ pass, Yt::Matrix4::perspective(pass.window_size(), 35, .5, 256), _state.camera_matrix() };
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

	void setCursor(const std::optional<Yt::Vector2>& cursor)
	{
		_cursor = cursor;
	}

private:
	GameState& _state;
	Model _cube;
	Model _checkerboard;
	std::optional<Yt::Vector2> _cursor;
};

class MinimapCanvas
{
public:
	explicit MinimapCanvas(GameState& state)
		: _state{ state } {}

	void draw(Yt::Renderer2D& renderer, const Yt::RectF& rect)
	{
		renderer.setTexture({});
		renderer.addRect(rect, Yt::Bgra32::grayscale(64, 192));
		if (_state._visible_area)
			renderer.addQuad(to_window(rect, *_state._visible_area), Yt::Bgra32::yellow(64));
		if (_cursor)
			renderer.addRect({ *_cursor, Yt::SizeF{ 1, 1 } }, Yt::Bgra32::green());
		renderer.addRect({ to_window(rect, { _state._position.x, _state._position.y }) - Yt::Vector2{ 2, 2 }, Yt::SizeF{ 4, 4 } }, Yt::Bgra32::red());
	}

	void setCursor(std::optional<Yt::Vector2>&& cursor, const Yt::RectF& rect)
	{
		_cursor = cursor;
		if (_cursor)
			_state.set_position(to_map(rect, *_cursor) - Yt::Vector2{ 0, 10 });
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

Game::Game(Yt::ResourceLoader& resourceLoader, Settings& settings)
	: _settings{ settings }
	, _state{ std::make_unique<GameState>() }
	, _world{ std::make_unique<WorldCanvas>(*_state, resourceLoader) }
	, _leftMinimap{ std::make_unique<MinimapCanvas>(*_state) }
	, _rightMinimap{ std::make_unique<MinimapCanvas>(*_state) }
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
	_settings.set("Camera", { Yt::make_string(_state->_position.x), Yt::make_string(_state->_position.y) });
}

Yt::Vector3 Game::cameraPosition() const noexcept
{
	return _state->_position;
}

std::optional<Yt::Vector2> Game::cursorCell() const noexcept
{
	return _state->_board_point;
}

void Game::mainScreen(Yt::GuiFrame& gui, Yt::Renderer2D& renderer, Yt::RenderPass& pass)
{
	const Yt::RectF viewportRect{ pass.window_size() };
	const auto logicalUnit = viewportRect.height() / 100;
	const auto minimapSize = 20 * logicalUnit;
	const Yt::RectF leftMinimapRect{ { logicalUnit, viewportRect.bottom() - minimapSize - logicalUnit }, Yt::SizeF{ minimapSize, minimapSize } };
	const Yt::RectF rightMinimapRect{ { viewportRect.right() - minimapSize - logicalUnit, viewportRect.bottom() - minimapSize - logicalUnit }, Yt::SizeF{ minimapSize, minimapSize } };
	_leftMinimap->setCursor(gui.dragArea("LeftMinimap", leftMinimapRect, Yt::Key::Mouse1), leftMinimapRect);
	_rightMinimap->setCursor(gui.dragArea("RightMinimap", rightMinimapRect, Yt::Key::Mouse1), rightMinimapRect);
	_world->setCursor(gui.hoverArea(viewportRect));
	_world->draw(pass);
	_rightMinimap->draw(renderer, rightMinimapRect);
	_leftMinimap->draw(renderer, leftMinimapRect);
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
