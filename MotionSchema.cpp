#include "MotionSchema.h"

#include "SimpleJson.h"

#include <cstring>
#include <fstream>
#include <windows.h>
#include <cmath>
#include <algorithm>

namespace
{
    const float kDefaultPos[3] = { 0.0f, 0.0f, 0.0f };
    const float kDefaultRot[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

    void NormalizeQuaternion(float* rot)
    {
        float length = std::sqrt(rot[0] * rot[0] + rot[1] * rot[1] + rot[2] * rot[2] + rot[3] * rot[3]);
        if (length <= 0.000001f) {
            rot[0] = 0.0f;
            rot[1] = 0.0f;
            rot[2] = 0.0f;
            rot[3] = 1.0f;
            return;
        }
        rot[0] /= length;
        rot[1] /= length;
        rot[2] /= length;
        rot[3] /= length;
    }

    bool ReadFloatArray(const JsonValue& value, float* output, size_t count)
    {
        if (!value.IsArray()) {
            return false;
        }
        for (size_t i = 0; i < count && i < value.array.size(); ++i) {
            if (value.array[i].IsNumber()) {
                output[i] = static_cast<float>(value.array[i].number);
            }
        }
        return true;
    }
}

bool ParseMotionJson(const std::string& json, MotionClip& clip)
{
    JsonValue root;
    if (!ParseJson(json, root, nullptr) || !root.IsObject()) {
        return false;
    }

    JsonValue canonicalRoot = root;
    const JsonValue* bonesValue = canonicalRoot.Find("bones");
    if (bonesValue && bonesValue->IsArray()) {
        for (auto& boneJson : canonicalRoot.object["bones"].array) {
            if (!boneJson.IsObject()) {
                continue;
            }
            JsonValue* nameValue = boneJson.Find("name") ? &boneJson.object["name"] : nullptr;
            if (!nameValue || !nameValue->IsString()) {
                continue;
            }
            std::string cleaned;
            for (char c : nameValue->string) {
                if (c != '"' && c != ',' && c != '\r' && c != '\n') {
                    cleaned.push_back(c);
                }
            }
            nameValue->string = cleaned;
        }
        root = canonicalRoot;
    }

    const JsonValue* boneFramesValue = root.Find("bone_frames");
    if (boneFramesValue && boneFramesValue->IsArray()) {
        for (auto& frameJson : root.object["bone_frames"].array) {
            if (!frameJson.IsObject()) {
                continue;
            }
            JsonValue* nameValue = frameJson.Find("bone_name") ? &frameJson.object["bone_name"] : nullptr;
            if (!nameValue || !nameValue->IsString()) {
                continue;
            }
            std::string cleaned;
            for (char c : nameValue->string) {
                if (c != '"' && c != ',' && c != '\r' && c != '\n') {
                    cleaned.push_back(c);
                }
            }
            nameValue->string = cleaned;
        }
    }

    const JsonValue* fpsValue = root.Find("fps");
    clip.fps = (fpsValue && fpsValue->IsNumber()) ? static_cast<int>(fpsValue->number) : 30;
    const JsonValue* durationValue = root.Find("duration");
    clip.duration = (durationValue && durationValue->IsNumber()) ? static_cast<int>(durationValue->number) : 0;
    const JsonValue* modelValue = root.Find("model_name");
    clip.modelName = (modelValue && modelValue->IsString()) ? modelValue->string : std::string();
    clip.bones.clear();

    if (boneFramesValue && boneFramesValue->IsArray()) {
        int maxFrame = 0;
        for (const auto& frameJson : boneFramesValue->array) {
            if (!frameJson.IsObject()) {
                continue;
            }
            const JsonValue* nameValue = frameJson.Find("bone_name");
            if (!nameValue || !nameValue->IsString() || nameValue->string.empty()) {
                continue;
            }
            MotionFrame frame;
            std::memcpy(frame.pos, kDefaultPos, sizeof(frame.pos));
            std::memcpy(frame.rot, kDefaultRot, sizeof(frame.rot));
            const JsonValue* frameValue = frameJson.Find("frame");
            if (frameValue && frameValue->IsNumber()) {
                frame.frame = static_cast<int>(frameValue->number);
            }
            if (frame.frame > maxFrame) {
                maxFrame = frame.frame;
            }
            const JsonValue* posValue = frameJson.Find("position");
            if (posValue) {
                ReadFloatArray(*posValue, frame.pos, 3);
            }
            const JsonValue* rotValue = frameJson.Find("rotation");
            if (rotValue) {
                ReadFloatArray(*rotValue, frame.rot, 4);
                NormalizeQuaternion(frame.rot);
            }

            auto existing = std::find_if(clip.bones.begin(), clip.bones.end(), [&](const MotionBone& bone) {
                return bone.name == nameValue->string;
            });
            if (existing == clip.bones.end()) {
                MotionBone bone;
                bone.name = nameValue->string;
                bone.frames.push_back(frame);
                clip.bones.push_back(std::move(bone));
            } else {
                existing->frames.push_back(frame);
            }
        }
        if (clip.duration <= 0) {
            clip.duration = maxFrame;
            if (clip.duration < 30) {
                clip.duration = 30;
            }
        }
        return !clip.bones.empty();
    }

    if (!bonesValue || !bonesValue->IsArray()) {
        return false;
    }

    for (const auto& boneJson : bonesValue->array) {
        if (!boneJson.IsObject()) {
            continue;
        }
        const JsonValue* nameValue = boneJson.Find("name");
        if (!nameValue || !nameValue->IsString() || nameValue->string.empty()) {
            continue;
        }
        const JsonValue* framesValue = boneJson.Find("frames");
        if (!framesValue || !framesValue->IsArray()) {
            continue;
        }
        MotionBone bone;
        bone.name = nameValue->string;

        for (const auto& frameJson : framesValue->array) {
            if (!frameJson.IsObject()) {
                continue;
            }
            MotionFrame frame;
            std::memcpy(frame.pos, kDefaultPos, sizeof(frame.pos));
            std::memcpy(frame.rot, kDefaultRot, sizeof(frame.rot));

            const JsonValue* fValue = frameJson.Find("f");
            if (fValue && fValue->IsNumber()) {
                frame.frame = static_cast<int>(fValue->number);
            }
            const JsonValue* posValue = frameJson.Find("pos");
            if (posValue) {
                ReadFloatArray(*posValue, frame.pos, 3);
            }
            const JsonValue* rotValue = frameJson.Find("rot");
            if (rotValue) {
                ReadFloatArray(*rotValue, frame.rot, 4);
                NormalizeQuaternion(frame.rot);
            }
            bone.frames.push_back(frame);
        }

        if (!bone.frames.empty()) {
            clip.bones.push_back(std::move(bone));
        }
    }

    return !clip.bones.empty();
}

void ExpandMissingFrames(MotionClip& clip, const std::vector<std::string>& boneNames)
{
    if (clip.duration <= 0 || clip.fps <= 0) {
        return;
    }

    int endFrame = clip.duration;
    for (const auto& boneName : boneNames) {
        auto existing = std::find_if(clip.bones.begin(), clip.bones.end(), [&](const MotionBone& bone) {
            return bone.name == boneName;
        });
        if (existing != clip.bones.end()) {
            bool hasStart = false;
            bool hasEnd = false;
            for (const auto& frame : existing->frames) {
                if (frame.frame == 0) {
                    hasStart = true;
                }
                if (frame.frame == endFrame) {
                    hasEnd = true;
                }
            }
            if (!hasStart) {
                MotionFrame startFrame;
                startFrame.frame = 0;
                existing->frames.push_back(startFrame);
            }
            if (!hasEnd) {
                MotionFrame endFrameData;
                endFrameData.frame = endFrame;
                existing->frames.push_back(endFrameData);
            }
            continue;
        }

        MotionBone bone;
        bone.name = boneName;
        MotionFrame startFrame;
        startFrame.frame = 0;
        MotionFrame endFrameData;
        endFrameData.frame = endFrame;
        bone.frames.push_back(startFrame);
        bone.frames.push_back(endFrameData);
        clip.bones.push_back(std::move(bone));
    }
}

std::vector<std::string> GetDefaultBoneNames()
{
  std::vector<std::string> result;
    std::ifstream input("BoneNames.txt", std::ios::binary);
    if (!input) {
        return result;
    }
    bool treatAsUtf8 = false;
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!treatAsUtf8 && line.size() >= 3 &&
            static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF) {
            line = line.substr(3);
            treatAsUtf8 = true;
        }
        if (!line.empty()) {
         if (treatAsUtf8) {
                result.push_back(line);
                continue;
            }
            int wideUtf8Size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, line.c_str(), static_cast<int>(line.size()), nullptr, 0);
            if (wideUtf8Size > 0) {
                result.push_back(line);
                continue;
            }
            int wideSize = MultiByteToWideChar(CP_ACP, 0, line.c_str(), static_cast<int>(line.size()), nullptr, 0);
            if (wideSize <= 0) {
                continue;
            }
            std::wstring wide(static_cast<size_t>(wideSize), L'\0');
            MultiByteToWideChar(CP_ACP, 0, line.c_str(), static_cast<int>(line.size()), wide.data(), wideSize);
            int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wideSize, nullptr, 0, nullptr, nullptr);
            if (utf8Size <= 0) {
                continue;
            }
            std::string utf8(static_cast<size_t>(utf8Size), '\0');
            WideCharToMultiByte(CP_UTF8, 0, wide.data(), wideSize, utf8.data(), utf8Size, nullptr, nullptr);
            result.push_back(utf8);
        }
    }
    return result;
}
