#pragma once
#include <string>

struct Animation
{
    std::string name;
    int row;
    int startFrame;
    int endFrame;
    float frameTime;
    bool loop;

    // Constructor for easy setup
    Animation(const std::string& n = "", int r = 0, int sf = 0, int ef = 0, float ft = 0.1f, bool l = true)
        : name(n), row(r), startFrame(sf), endFrame(ef), frameTime(ft), loop(l) {
    }
};