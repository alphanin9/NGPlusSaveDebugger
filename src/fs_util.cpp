#include <algorithm>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <unordered_set>

#include "Windows.h"
#include "shlobj_core.h"

#include <simdjson.h>

#include "fs_util.hpp"

#include "context.hpp"

namespace files
{
    const std::filesystem::path& GetCpSaveFolder()
    {
        PWSTR userProfilePath{};

        // NOTE: CP2077 doesn't actually seem to use FOLDERID_SavedGames - does current method play nice with other language
        // versions of Windows?
        SHGetKnownFolderPath(FOLDERID_Profile, KF_FLAG_CREATE, nullptr, &userProfilePath);
        if (!userProfilePath)
        {
            static const std::filesystem::path errorPath{};

            Context::Error("FOLDERID_Profile could not be found!");
            return errorPath;
        }

        static const std::filesystem::path basePath = userProfilePath;
        static const std::filesystem::path finalPath = basePath / L"Saved Games" / L"CD Projekt Red" / L"Cyberpunk 2077";

        CoTaskMemFree(userProfilePath);

        return finalPath;
    }

    // Too lazy to bother with anything older LOL
    constexpr std::int64_t minSupportedGameVersion = 2000;

    // No longer checks for PONR, needs a name change
    bool HasValidPointOfNoReturnSave()
    {
        static const auto& basePath = GetCpSaveFolder();

        std::filesystem::directory_iterator directoryIterator{ basePath };

        // Debug version is greedy and doesn't stop on one save...

        auto foundSave = false;

        for (auto& i : directoryIterator)
        {
            if (!i.is_directory())
            {
                continue;
            }

            const auto saveName = i.path().stem().string();

            if (IsValidForNewGamePlus(saveName))
            {
                foundSave = true;
            }
        }

        return foundSave;
    }

    bool IsValidForNewGamePlus(std::string_view aSaveName, uint64_t& aPlaythroughHash) noexcept
    {
        static const auto& basePath = GetCpSaveFolder();

        constexpr auto shouldDebugMetadataValidation = true;

        if (aSaveName.starts_with("EndGameSave"))
        {
            // Hack, not sure if necessary...
            return false;
        }

        const auto metadataPath = basePath / aSaveName / "metadata.9.json";

        if constexpr (shouldDebugMetadataValidation)
        {
            Context::Spew("Processing save {}...", std::move(metadataPath.string()));
        }

        std::error_code ec{};

        if (!std::filesystem::is_regular_file(metadataPath, ec))
        {
            if constexpr (shouldDebugMetadataValidation)
            {
                Context::Spew("\tMetadata is not regular file!");
            }
            return false;
        }

        auto padded = simdjson::padded_string::load(metadataPath.string());

        if (padded.error() != simdjson::SUCCESS)
        {
            if constexpr (shouldDebugMetadataValidation)
            {
                Context::Spew("\tFailed to load metadata into padded string!");
            }
            return false;
        }

        simdjson::dom::parser parser{};
        simdjson::dom::element document{};

        if (parser.parse(padded.value()).get(document) != simdjson::SUCCESS)
        {
            if constexpr (shouldDebugMetadataValidation)
            {
                Context::Spew("\tFailed to parse metadata!");
            }
            return false;
        }

        if (!document.is_object())
        {
            if constexpr (shouldDebugMetadataValidation)
            {
                Context::Spew("\tMetadata is not object!");
            }
            return false;
        }

        if (!document.at_key("RootType").is_string())
        {
            if constexpr (shouldDebugMetadataValidation)
            {
                Context::Spew("\tMissing RootType!");
            }
            return false;
        }

        simdjson::dom::element saveMetadata{};

        if (document.at_key("Data").at_key("metadata").get(saveMetadata) != simdjson::SUCCESS)
        {
            if constexpr (shouldDebugMetadataValidation)
            {
                Context::Spew("\tInner metadata not found!");
            }
            return false;
        }

        // ISSUE: old saves (might not be from PC?) don't have save metadata correct
        // Switch to noexcept versions

        int64_t gameVersion{};

        if (saveMetadata.at_key("gameVersion").get_int64().get(gameVersion) != simdjson::SUCCESS)
        {
            if constexpr (shouldDebugMetadataValidation)
            {
                Context::Spew("\tBad game version!");
            }
            return false;
        }

        if (minSupportedGameVersion > gameVersion)
        {
            if constexpr (shouldDebugMetadataValidation)
            {
                Context::Spew("\tSave is too old, minimum supported game version: {}, save game version: {}", minSupportedGameVersion, gameVersion);
            }
            return false;
        }

        const auto isPointOfNoReturn = aSaveName.starts_with("PointOfNoReturn");

        std::string_view playthroughId{};

        if (saveMetadata.at_key("playthroughID").get_string().get(playthroughId) != simdjson::SUCCESS)
        {
            if constexpr (shouldDebugMetadataValidation)
            {
                Context::Spew("\tplaythroughID not found!");
            }
            return false;
        }

        // This version does not use playthrough ID filtering...
        if (isPointOfNoReturn)
        {
            Context::Spew("\tSave is PONR and thus good, but debug necessitates more checks...");
        }

        std::string_view questsDone{};

        if (saveMetadata.at_key("finishedQuests").get_string().get(questsDone) != simdjson::SUCCESS)
        {
            if constexpr (shouldDebugMetadataValidation)
            {
                Context::Spew("\tfinishedQuests not found!");
            }
            return false;
        }

        using std::operator""sv;
        auto questsSplitRange = std::views::split(questsDone, " "sv);

        std::unordered_set<std::string_view> questsSet(questsSplitRange.begin(), questsSplitRange.end());

        if (questsSet.contains("q104") && questsSet.contains("q110") && questsSet.contains("q112"))
        {
            if constexpr (shouldDebugMetadataValidation)
            {
                Context::Spew("\tNecessary quests done, NG+ can be done!");
            }
            return true;
        }

        constexpr auto q307ActiveFact = "q307_blueprint_acquired=1";

        simdjson::dom::array importantFacts{};

        if (saveMetadata.at_key("facts").get_array().get(importantFacts) != simdjson::SUCCESS)
        {
            if constexpr (shouldDebugMetadataValidation)
            {
                Context::Spew("\tImportant facts not found!");
            }
            return false;
        }

        for (const auto& fact : importantFacts)
        {
            std::string_view factValueString{};

            if (fact.get_string().get(factValueString) != simdjson::SUCCESS)
            {
                continue;
            }

            if (factValueString == q307ActiveFact)
            {
                if constexpr (shouldDebugMetadataValidation)
                {
                    Context::Spew("\tSave has Q307 available, good!");
                }
                return true;
            }
        }

        if constexpr (shouldDebugMetadataValidation)
        {
            Context::Spew("\tSave does not meet criteria!");
        }
        return false;
    }

    bool IsValidForNewGamePlus(std::string_view aSaveName) noexcept
    {
        uint64_t dummy{};
        return IsValidForNewGamePlus(aSaveName, dummy);
    }
} // namespace files