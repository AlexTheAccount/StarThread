#include "../ENGINE/EngineComponents.h"
#include "../IMGUI/ImguiComponents.h"

namespace ENGINE
{

    void EnsureMusicLooping(entt::registry& registry)
    {
        if (auto gMusicPtr = registry.ctx().find<GW::AUDIO::GMusic>())
        {
            bool playing = false;
            gMusicPtr->isPlaying(playing);
            if (!playing)
            {
                gMusicPtr->Play();
            }
        }
    }

    void UpdateAudioVolumes(entt::registry& registry)
    {
        EnsureMusicLooping(registry);

        // Ensure AudioSettings has been emplaced
        auto* settingsPtr = registry.ctx().find<ENGINE::AudioSettings>();
        if (!settingsPtr)
            return;

        float newMain = settingsPtr->mainVolume;
        float newMusic = settingsPtr->musicVolume;
        float newSFX = settingsPtr->sfxVolume;

        // get FloatSlider components
        auto view = registry.view<UI::FloatSlider>();
        for (auto entity : view)
        {
            auto& slider = registry.get<UI::FloatSlider>(entity);
            if (slider.label == "Main Volume")
                newMain = slider.value;
            else if (slider.label == "Music Volume")
                newMusic = slider.value;
            else if (slider.label == "SFX Volume")
                newSFX = slider.value;
        }

        // If nothing changed, do nothing
        if (newMain == settingsPtr->mainVolume && newMusic == settingsPtr->musicVolume && newSFX == settingsPtr->sfxVolume)
            return;

        // Update stored settings
        settingsPtr->mainVolume = newMain;
        settingsPtr->musicVolume = newMusic;
        settingsPtr->sfxVolume = newSFX;

        // Apply main volume to GAudio
        if (auto gAudioPtr = registry.ctx().find<GW::AUDIO::GAudio>())
        {
            gAudioPtr->SetMasterVolume(settingsPtr->mainVolume);
        }

        // Apply main volume to background music if available
        if (auto gMusicPtr = registry.ctx().find<GW::AUDIO::GMusic>())
        {
            float effectiveMusicVolume = settingsPtr->mainVolume * settingsPtr->musicVolume;
            gMusicPtr->SetVolume(effectiveMusicVolume);
        }

        // Apply SFX volumes via SoundBank if present
        if (auto SFXBank = registry.ctx().find<ENGINE::SoundBank>())
        {
            float effectiveSFXVolume = settingsPtr->mainVolume * settingsPtr->sfxVolume;
            for (auto& soundPair : SFXBank->sounds)
            {
                if (soundPair.second)
                {
                    soundPair.second->SetVolume(effectiveSFXVolume);
                }
            }
        }
    }
}