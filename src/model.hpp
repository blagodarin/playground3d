// This file is part of Playground3D.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <string_view>

namespace Yt
{
	class Material;
	class Mesh;
	class RenderPass;
	class ResourceLoader;
}

class Model
{
public:
	Model(Yt::ResourceLoader&, std::string_view mesh, std::string_view material);
	~Model();

	void draw(Yt::RenderPass&);

private:
	const std::shared_ptr<const Yt::Mesh> _mesh;
	const std::shared_ptr<const Yt::Material> _material;
};
