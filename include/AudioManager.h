#pragma once
#include <SFML/Audio.hpp>
#include <unordered_map>
#include <string>
#include <list>

class AudioManager
{
public:
    AudioManager() = default;

    void LoadSound(const std::string& id, const std::string& path);
    void PlaySound(const std::string& id);
    void PlayMusic(const std::string& path);
    void StopMusic();
    void Update();
    void SetMasterVolume(float volume);
    float GetMasterVolume() const { return m_masterVolume; }

private:
    std::unordered_map<std::string, sf::SoundBuffer> m_soundBuffers;
    std::list<sf::Sound> m_activeSounds;
    sf::Music m_music;
    float m_masterVolume = 100.0f;
};