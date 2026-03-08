#include <iostream>
#include <algorithm>
#include "AudioManager.h"
#include "Utilities.h"
#include "EngineLog.h"

void AudioManager::LoadSound(const std::string& id, const std::string& path)
{
    sf::SoundBuffer buffer;
    const std::string fullPath = Utils::GetWorkingDirectory() + path;

    if (buffer.loadFromFile(fullPath))
        m_soundBuffers[id] = std::move(buffer);
    else
        EngineLog::Error("Failed to load sound: " + fullPath);
}

void AudioManager::PlaySound(const std::string& id)
{
    auto it = m_soundBuffers.find(id);
    if (it != m_soundBuffers.end())
    {
        m_activeSounds.emplace_back(it->second);
        m_activeSounds.back().setVolume(m_masterVolume);
        m_activeSounds.back().play();
    }
}

void AudioManager::PlayMusic(const std::string& path)
{
    const std::string fullPath = Utils::GetWorkingDirectory() + path;

    if (m_music.openFromFile(fullPath))
    {
        m_music.setLooping(true);
        m_music.setVolume(m_masterVolume);
        m_music.play();
    }
    else
        EngineLog::Error("Failed to load music: " + fullPath);
}

void AudioManager::StopMusic()
{
    m_music.stop();
}

void AudioManager::Update()
{
    m_activeSounds.remove_if([](const sf::Sound& sound)
        {
            return sound.getStatus() == sf::SoundSource::Status::Stopped;
        }
    );
}

void AudioManager::SetMasterVolume(float volume)
{
    m_masterVolume = std::clamp(volume, 0.0f, 100.0f);

    m_music.setVolume(m_masterVolume);
    for (auto& sound : m_activeSounds)
        sound.setVolume(m_masterVolume);
}