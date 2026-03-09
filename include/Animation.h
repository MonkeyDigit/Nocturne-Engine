#pragma once
#include <string>
#include <istream>

class Animation
{
public:
    Animation();
    ~Animation() = default;

    void Configure(
        const std::string& name,
        unsigned int startFrame,
        unsigned int endFrame,
        unsigned int row,
        float frameTime,
        bool loop);

    void Play();
    void Stop();
    void SetLooping(bool loop);
    void Update(float deltaTime);

    friend std::istream& operator>>(std::istream& is, Animation& a);

    const std::string& GetName() const;
    unsigned int GetCurrentFrame() const;
    unsigned int GetRow() const;
    bool IsPlaying() const;
    bool IsLooped() const;

private:
    std::string m_name;
    unsigned int m_startFrame;
    unsigned int m_endFrame;
    unsigned int m_currentFrame;
    unsigned int m_frameRow;
    float m_frameTime;

    bool m_playing;
    bool m_loop;
    float m_elapsedTime;
};