#include <memory>
#include "../../gateware-26.33.16/Gateware.h"

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
        // constructor loads game settings, writes defaults if none exist
        GameConfig();
        // destructor saves current game settings between plays
        virtual ~GameConfig();

        // Loads configuration from the specified file
        void load(const char* filename);

        // Saves configuration to the specified file
        void save(const char* filename);

        std::unordered_map<std::string, std::string> entries;
    };

    //*** COMPONENTS ***//
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