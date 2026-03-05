#include "AudioManager.h"
#include <iostream>

void AudioManager::LoadSound(const std::string& id, const std::string& path)
{
    sf::SoundBuffer buffer;
    if (buffer.loadFromFile(path))
    {
        m_soundBuffers[id] = std::move(buffer);
    }
    else
    {
        std::cerr << "! Failed to load sound: " << path << '\n';
    }
}

void AudioManager::PlaySound(const std::string& id)
{
    auto it = m_soundBuffers.find(id);
    if (it != m_soundBuffers.end())
    {
        m_activeSounds.emplace_back(it->second);
        m_activeSounds.back().play();
    }
}

void AudioManager::PlayMusic(const std::string& path)
{
    if (m_music.openFromFile(path))
    {
        m_music.setLooping(true);
        m_music.play();
    }
    else
    {
        std::cerr << "! Failed to load music: " << path << '\n';
    }
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