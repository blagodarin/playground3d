// This file is part of Playground3D.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <filesystem>
#include <map>
#include <string>
#include <vector>

class Settings
{
public:
	Settings(const std::filesystem::path&);
	~Settings() { save(); }

	std::vector<std::string> get(std::string_view key) const;
	void save();
	void set(const std::string& key, const std::vector<std::string>& values);

private:
	const std::filesystem::path _path;
	std::map<std::string, std::pair<bool, std::vector<std::string>>, std::less<>> _settings;
};
