#pragma once

#include <filesystem>
namespace files {
	const std::filesystem::path& GetCpSaveFolder();
	bool HasValidPointOfNoReturnSave();
	bool IsValidForNewGamePlus(std::string_view aSaveName) noexcept;
	bool IsValidForNewGamePlus(std::string_view aSaveName, uint64_t& aPlaythroughHash) noexcept;
}