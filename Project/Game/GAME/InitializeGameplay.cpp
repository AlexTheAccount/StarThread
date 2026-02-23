
#include "../GAME/GameComponents.h"
#include "../../Engine/IMGUI/ImguiComponents.h"
using namespace ENGINE;

namespace GAME
{
    void InitializeGameplay(entt::registry& registry)
    {
        std::shared_ptr<const GameConfig> config = registry.ctx().get<UTILITIES::Config>().gameConfig;

        // Create the Audio System
        using namespace GW::AUDIO;
        GAudio& gAudio = registry.ctx().emplace<GAudio>();
        gAudio.Create();
        gAudio.SetMasterVolume(1.0f);

        // create sound bank
        SoundBank& sfx = registry.ctx().emplace<SoundBank>();
        sfx.Initialize(gAudio);

        // load SFX files
        sfx.Load("button", "../Audio/button.wav");
        sfx.Load("move", "../Audio/move.wav");
        sfx.Load("rotate", "../Audio/rotate.wav");
        sfx.Load("harddrop", "../Audio/hardDrop.wav");
        sfx.Load("lock", "../Audio/lock.wav");
        sfx.Load("lineclear", "../Audio/lineclear.wav");

        // background music
        GMusic& gMusic = registry.ctx().emplace<GMusic>();
        gMusic.Create("../Audio/bg_music.wav", gAudio);
        gMusic.Play();

        registry.ctx().emplace<AudioSettings>(AudioSettings{ 1.0f, 1.0f, 1.0f });

        registry.ctx().emplace<UTILITIES::XInputVibration>();


        // fill out the MeshCollection of both the Player and the Enemy
        RENDERING::ModelManager* mmPtr = nullptr;
        auto mmView = registry.view<RENDERING::ModelManager>();
        if (!mmView.empty())
            mmPtr = &registry.get<RENDERING::ModelManager>(mmView.front());
    }
}