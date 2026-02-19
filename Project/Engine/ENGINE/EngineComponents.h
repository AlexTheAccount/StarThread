#pragma once
#include <entt/entt.hpp>

namespace ENGINE
{
    inline entt::registry& GlobalRegistry()
    {
        static entt::registry registry;
        return registry;
    }
    
    struct SoundBank
    {
        GW::AUDIO::GAudio* audio;
        std::unordered_map<std::string, std::unique_ptr<GW::AUDIO::GSound>> sounds;

        // allow default construction
        SoundBank() = default;

        // disable copying
        SoundBank(const SoundBank&) = delete;
        SoundBank& operator=(const SoundBank&) = delete;

        // allow moving
        SoundBank(SoundBank&&) noexcept = default;
        SoundBank& operator=(SoundBank&&) noexcept = default;

        void Initialize(GW::AUDIO::GAudio& gAudio)
        {
            audio = &gAudio;
        }

        bool Load(const std::string& name, const char* path)
        {
            if (audio == nullptr) return false;
            auto sound = std::make_unique<GW::AUDIO::GSound>();
            sound->Create(path, *audio);
            sounds.emplace(name, std::move(sound));
            return true;
        }

        void Play(const std::string& name, bool loop = false)
        {
            auto it = sounds.find(name);
            if (it == sounds.end()) return;
            it->second->Play();
        }
    };
}