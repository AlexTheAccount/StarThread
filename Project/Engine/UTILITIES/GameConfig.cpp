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

        // Clear both the simple entries map and the rapidjson document
        entries.clear();
        document.SetObject();

        std::string line;
        std::string currentSection;
        rapidjson::Document::AllocatorType& alloc = document.GetAllocator();

        while (std::getline(readFile, line))
        {
            // strip comments
            size_t commentLocation = line.find_first_of(";#");
            if (commentLocation != std::string::npos) line.erase(commentLocation);

            line = trim(line);
            if (line.empty()) continue;

            // Section header: [Section]
            if (line.front() == '[' && line.back() == ']')
            {
                currentSection = trim(line.substr(1, line.size() - 2));
                if (!document.HasMember(currentSection.c_str()))
                {
                    rapidjson::Value secName(currentSection.c_str(), alloc);
                    rapidjson::Value secObj(rapidjson::kObjectType);
                    document.AddMember(secName, secObj, alloc);
                }
                continue;
            }

            size_t equal = line.find('=');
            if (equal == std::string::npos) continue;

            std::string key = trim(line.substr(0, equal));
            std::string value = trim(line.substr(equal + 1));

            if (key.empty() || currentSection.empty()) continue;

            // Add to entries map
            entries.emplace(currentSection + "." + key, value);

            // Prepare rapidjson values
            rapidjson::Value rapidKey(key.c_str(), alloc);
            rapidjson::Value rapidValue;

            // Try boolean
            std::string lower = value;
            for (auto& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (lower == "true" || lower == "false")
            {
                rapidValue.SetBool(lower == "true");
            }
            else
            {
                // Try numeric (integer or double)
                bool isNumber = true;
                bool hasDot = false;
                size_t start = 0;
                if (value.size() > 0 && (value[0] == '-' || value[0] == '+')) start = 1;
                for (size_t index = start; index < value.size(); ++index)
                {
                    if (value[index] == '.') { if (hasDot) { isNumber = false; break; } hasDot = true; continue; }
                    if (!std::isdigit(static_cast<unsigned char>(value[index]))) { isNumber = false; break; }
                }

                if (isNumber && !value.empty())
                {
                    try
                    {
                        if (hasDot)
                        {
                            double d = std::stod(value);
                            rapidValue.SetDouble(d);
                        }
                        else
                        {
                            int index = std::stoi(value);
                            rapidValue.SetInt(index);
                        }
                    }
                    catch (...)
                    {
                        rapidValue.SetString(value.c_str(), alloc);
                    }
                }
                else
                {
                    rapidValue.SetString(value.c_str(), alloc);
                }
            }

            // Ensure section object exists, then add member
            rapidjson::Value& sectionObject = document[currentSection.c_str()];
            sectionObject.AddMember(rapidKey, rapidValue, alloc);
        }
    }

    void GameConfig::save(const char* filename)
    {
        std::ofstream outFile(filename, std::ios::out | std::ios::trunc);
        if (!outFile.is_open()) return;

        for (auto second = document.MemberBegin(); second != document.MemberEnd(); ++second)
        {
            outFile << '[' << second->name.GetString() << ']' << '\n';
            if (second->value.IsObject())
            {
                for (auto member = second->value.MemberBegin(); member != second->value.MemberEnd(); ++member)
                {
                    outFile << member->name.GetString() << '=';
                    if (member->value.IsString())
                        outFile << member->value.GetString();
                    else if (member->value.IsBool())
                        outFile << (member->value.GetBool() ? "true" : "false");
                    else if (member->value.IsInt())
                        outFile << member->value.GetInt();
                    else if (member->value.IsDouble())
                        outFile << member->value.GetDouble();
                    else
                        outFile << ""; // fallback

                    outFile << '\n';
                }
            }
            outFile << '\n';
        }
        outFile.close();
    }
}