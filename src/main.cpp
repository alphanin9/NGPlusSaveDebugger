#include "context.hpp"
#include "fs_util.hpp"

#include "Windows.h"
#include "shlobj_core.h"

#include <cstdio>

void SpewSavedGamesPaths() noexcept {
    const auto& saveFolder = files::GetCpSaveFolder();

    Context::Spew("Save folder: {}", std::move(saveFolder.string()));

    // This is no longer used by NG+, but still worth checking methinks

    PWSTR savedGamesPath{};

    SHGetKnownFolderPath(FOLDERID_SavedGames, KF_FLAG_CREATE, nullptr, &savedGamesPath);
    if (!savedGamesPath)
    {
        Context::Error("Failed to get old saved games folder!");
        return;
    }

    std::filesystem::path fsPath{ savedGamesPath };
    CoTaskMemFree(savedGamesPath);
    Context::Spew("Windows saved games folder: {}", std::move(fsPath.string()));
}

int main() noexcept {
    Context::m_stream = std::ofstream("./NGPlusLog.txt");
    Context::m_streamOpen = true;

    Context::Spew("Running NG+ save metadata debug tool...");

    SpewSavedGamesPaths();

    const auto hasNGPlusSaves = files::HasValidPointOfNoReturnSave();

    Context::Spew("NG+ save status: {}", hasNGPlusSaves);

    Context::Spew("Press any key to exit...");
    auto _ = std::getchar();

    Context::m_streamOpen = false;

    return 0;
}