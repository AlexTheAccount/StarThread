#pragma once
#include <memory>
#include "../../gateware-26.33.16/Gateware.h"
#include "../../_deps/assimp-src/contrib/rapidjson/include/rapidjson/document.h"


namespace UTILITIES
{
    //*** CLASSES ***//
    class XInputVibration
    {
    public:
        XInputVibration();
        void StartVibration(unsigned int controllerIndex, float leftMotor, float rightMotor, int durationMS);
        void Update(float deltaSeconds);

    private:
        struct State { float left; float right; float remaining; };
        std::array<State, 4> states;
    };

    class GameConfig
    {
    public:
        class ValueAccessor
        {
        private:
            const rapidjson::Value* value;

        public:
            ValueAccessor(const rapidjson::Value* value) : value(value) {}

            ValueAccessor at(const std::string& key) const
            {
                if (!value || !value->IsObject())
                    throw std::runtime_error("Value is not an object");

                if (!value->HasMember(key.c_str()))
                    throw std::runtime_error("Key not found: " + key);

                return ValueAccessor(&(*value)[key.c_str()]);
            }

            template<typename T>
            T as() const;
        };

        // constructor loads game settings, writes defaults if none exist
        GameConfig();
        // destructor saves current game settings between plays
        virtual ~GameConfig();

        // Loads configuration from the specified file
        void load(const char* filename);

        // Saves configuration to the specified file
        void save(const char* filename);

        // Access configuration value
        ValueAccessor at(const std::string& section) const
        {
            if (!document.HasMember(section.c_str()))
                throw std::runtime_error("Section not found: " + section);

            return ValueAccessor(&document[section.c_str()]);
        }

        std::unordered_map<std::string, std::string> entries;

    private:
        rapidjson::Document document;
    };

    // *** TEMPLATE SPECIALIZATIONS *** //
    template<>
    inline float GameConfig::ValueAccessor::as<float>() const
    {
        if (!value)
            throw std::runtime_error("Value is null");

        if (value->IsDouble())
            return static_cast<float>(value->GetDouble());
        else if (value->IsInt())
            return static_cast<float>(value->GetInt());
        else if (value->IsUint())
            return static_cast<float>(value->GetUint());

        throw std::runtime_error("Value is not a number");
    }

    template<>
    inline int GameConfig::ValueAccessor::as<int>() const
    {
        if (!value)
            throw std::runtime_error("Value is null");

        if (value->IsInt())
            return value->GetInt();

        throw std::runtime_error("Value is not an integer");
    }

    template<>
    inline std::string GameConfig::ValueAccessor::as<std::string>() const
    {
        if (!value)
            throw std::runtime_error("Value is null");

        if (value->IsString())
            return std::string(value->GetString());

        throw std::runtime_error("Value is not a string");
    }

    //*** COMPONENTS ***//
    struct float2
    {
        float2(float xy) : x(xy), y(xy) {};
        float2(float x, float y) : x(x), y(y) {};

        float x, y;
    };

	struct Config
	{
		std::shared_ptr<GameConfig> gameConfig = std::make_shared<GameConfig>();
	};

	struct DeltaTime
	{
		double dtSec;
	};

	struct Input
	{
		GW::INPUT::GController gamePads;         // controller support
		GW::INPUT::GInput immediateInput;        // twitch keybaord/mouse
		GW::INPUT::GBufferedInput bufferedInput; // event keyboard/mouse
	};

    struct FBXVertex
    {
        float position[3];
        float normal[3];
        float uv[2];
    };

    // *** FUNCTIONS *** //
    bool LoadFBXMesh(const std::string& filepath, std::vector<FBXVertex>& outVertices, std::vector<uint32_t>& outIndices);

}