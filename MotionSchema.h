#pragma once

#include <string>
#include <vector>

struct MotionFrame
{
    int frame = 0;
    float pos[3] = { 0.0f, 0.0f, 0.0f };
    float rot[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
};

struct MotionBone
{
    std::string name;
    std::vector<MotionFrame> frames;
};

struct MotionClip
{
    int fps = 30;
    int duration = 0;
  std::string modelName;
    std::vector<MotionBone> bones;
};

bool ParseMotionJson(const std::string& json, MotionClip& clip);

std::vector<std::string> GetDefaultBoneNames();

void ExpandMissingFrames(MotionClip& clip, const std::vector<std::string>& boneNames);
