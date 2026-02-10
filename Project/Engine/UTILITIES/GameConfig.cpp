#include "UtilityComponents.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cctype>

using namespace std::chrono_literals;
using namespace std::filesystem;

// helpers
static inline std::string trim(const std::string& characters)
{
    size_t begin = 0;
    size_t end = characters.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(characters[begin]))) ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(characters[end - 1]))) --end;
    return characters.substr(begin, end - begin);
}

namespace UTILITIES
{
    GameConfig::GameConfig()
    {

        // Resolve executable directory
        char exePath[MAX_PATH] = {};
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();

        // Resolve config paths
        std::filesystem::path configDir = exeDir.parent_path().parent_path() / "Config";
        std::filesystem::path defaultsPath = configDir / "defaults.ini";
        std::filesystem::path savedPath = configDir / "saved.ini";

        printf("Resolved defaults path = %s\n", defaultsPath.string().c_str());

        // Load the .ini file from disk, if it exists, otherwise create a new one with default values
        std::string defaultsString = defaultsPath.string();
        std::string savedString = savedPath.string();
        const char* defaults = defaultsString.c_str();
        const char* saved = savedString.c_str();

        // if they both exist choose the newest one
        if (exists(defaults) &&
            exists(saved))
        {
            // Load the newer file
            file_time_type defaultTime = last_write_time(defaults);
            file_time_type savedTime = last_write_time(saved);
            if (defaultTime > savedTime)
            {   // defaults were modified
                (*this).load(defaults);
            }
            else
            {   // saved was modified
                (*this).load(saved);
            }
        }
        else if (exists(defaults))
        {   // the first run after install
            (*this).load(defaults);
        }
        else
        {   // the default file is missing or corrupted
            std::abort();
        }
    }

    GameConfig::~GameConfig()
    {
        // Save current state to disk
        (*this).save("../Config/saved.ini");
    }

    void GameConfig::load(const char* filename)
    {
        std::ifstream readFile(filename, std::ios::in);
        if (!readFile.is_open()) return;

        entries.clear();
        std::string line;
        while (std::getline(readFile, line))
        {
            // strip comments
            size_t commentLocation = line.find_first_of(";#");
            if (commentLocation != std::string::npos) line.erase(commentLocation);

            size_t equal = line.find('=');
            if (equal == std::string::npos) continue;

            std::string key = trim(line.substr(0, equal));
            std::string value = trim(line.substr(equal + 1));
            if (!key.empty())
                entries.emplace(std::move(key), std::move(value));
        }
    }

    void GameConfig::save(const char* filename)
    {
        std::ofstream outFile(filename, std::ios::out | std::ios::trunc);
        if (!outFile.is_open()) return;

        for (const std::pair<const std::string, std::string>& entry : entries)
        {
            outFile << entry.first << '=' << entry.second << '\n';
        }
        outFile.close();
    }
}