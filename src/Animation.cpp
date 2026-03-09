#include "Animation.h"

Animation::Animation()
    : m_startFrame(0),
    m_endFrame(0),
    m_currentFrame(0),
    m_frameRow(0),
    m_frameTime(0.f),
    m_playing(false),
    m_loop(false),
    m_elapsedTime(0.f)
{
}

void Animation::Configure(
    const std::string& name,
    unsigned int startFrame,
    unsigned int endFrame,
    unsigned int row,
    float frameTime,
    bool loop)
{
    // Configure a full animation definition in one place
    m_name = name;
    m_startFrame = startFrame;
    m_endFrame = endFrame;
    m_frameRow = row;
    m_frameTime = frameTime;
    m_loop = loop;

    // Reset runtime state so reused Animation objects are deterministic
    m_currentFrame = m_startFrame;
    m_elapsedTime = 0.0f;
    m_playing = false;
}

void Animation::Play() { m_playing = true; }

void Animation::Stop()
{
    m_playing = false;
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
        m_elapsedTime -= m_frameTime;
        m_currentFrame++;

        if (m_currentFrame > m_endFrame)
        {
            if (m_loop)
            {
                m_currentFrame = m_startFrame;
            }
            else
            {
                m_currentFrame = m_endFrame;
                m_playing = false;
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
    std::string name;
    unsigned int startFrame = 0;
    unsigned int endFrame = 0;
    unsigned int row = 0;
    float frameTime = 0.0f;
    int loop = 0;

    // Expected format: Animation Name StartFrame EndFrame Row FrameTime Loop
    if (!(is >> type >> name >> startFrame >> endFrame >> row >> frameTime >> loop))
        return is;

    if (type != "Animation" ||
        name.empty() ||
        endFrame < startFrame ||
        frameTime <= 0.0f ||
        (loop != 0 && loop != 1))
    {
        is.setstate(std::ios::failbit);
        return is;
    }

    a.Configure(name, startFrame, endFrame, row, frameTime, loop == 1);
    return is;
}