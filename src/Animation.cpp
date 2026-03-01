#include "Animation.h"

Animation::Animation()
    : m_startFrame(0), m_endFrame(0), m_currentFrame(0),
    m_frameRow(0), m_frameTime(0.f), m_playing(false),
    m_loop(false), m_elapsedTime(0.f) {
}

void Animation::Play() { m_playing = true; }

void Animation::Stop()
{
    m_playing = false;
    // TODO: TOGLIERE STA ROBA ?
    m_currentFrame = m_startFrame;
    m_elapsedTime = 0.0f;
}

void Animation::SetLooping(bool loop) { m_loop = loop; }

void Animation::Update(float deltaTime)
{
    if (!m_playing) return;

    m_elapsedTime += deltaTime;
    if (m_elapsedTime >= m_frameTime)
    {
        m_elapsedTime = 0.0f;
        m_currentFrame++;

        if (m_currentFrame > m_endFrame)
        {
            if (m_loop)
            {
                m_currentFrame = m_startFrame;
            }
            else
            {
                m_currentFrame = m_endFrame;    // Lock the last frame
                m_playing = false;              // Stop animation
            }
        }
    }
}

const std::string& Animation::GetName() const { return m_name; }
unsigned int Animation::GetCurrentFrame() const { return m_currentFrame; }
unsigned int Animation::GetRow() const { return m_frameRow; }
bool Animation::IsPlaying() const { return m_playing; }
bool Animation::IsLooped() const { return m_loop; }

std::istream& operator>>(std::istream& is, Animation& a)
{
    std::string type;
    // Expected format: Animation Name StartFrame EndFrame Row FrameTime Loop
    is >> type >> a.m_name >> a.m_startFrame >> a.m_endFrame >> a.m_frameRow >> a.m_frameTime >> a.m_loop;
    return is;
}