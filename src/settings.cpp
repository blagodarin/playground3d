// This file is part of Playground3D.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "settings.hpp"

#include <yttrium/base/exceptions.h>
#include <yttrium/ion/reader.h>
#include <yttrium/ion/writer.h>
#include <yttrium/storage/source.h>
#include <yttrium/storage/writer.h>

Settings::Settings(const std::filesystem::path& path)
	: _path{ path }
{
	if (const auto source = Yt::Source::from(_path))
	{
		try
		{
			Yt::IonReader ion{ *source };
			auto token = ion.read();
			while (token.type() != Yt::IonToken::Type::End)
			{
				auto& item = _settings[std::string{ token.to_name() }];
				for (;;)
				{
					token = ion.read();
					if (token.type() != Yt::IonToken::Type::StringValue)
						break;
					item.second.emplace_back(token.to_value());
				}
			}
		}
		catch (const Yt::IonError&)
		{
			_settings.clear();
		}
	}
}

std::vector<std::string> Settings::get(std::string_view key) const
{
	if (const auto i = _settings.find(key); i != _settings.end())
		return i->second.second;
	return {};
}

void Settings::save()
{
	Yt::Writer writer{ _path };
	Yt::IonWriter ion{ writer, Yt::IonWriter::Formatting::Pretty };
	for (const auto& [key, item] : _settings)
	{
		if (item.first)
		{
			ion.add_name(key);
			for (const auto& value : item.second)
				ion.add_value(value);
		}
	}
	ion.flush();
}

void Settings::set(const std::string& key, const std::vector<std::string>& values)
{
	auto& item = _settings[key];
	item.first = true;
	item.second = values;
}
