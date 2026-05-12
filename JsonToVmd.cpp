#include "JsonToVmd.h"

#include <fstream>
#include <vector>
#include <cstring>
#include <windows.h>
#include <algorithm>

namespace
{
    std::string Utf8ToSystem(const std::string& value)
    {
        if (value.empty()) {
            return {};
        }
        int wideSize = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
        if (wideSize <= 0) {
            return value;
        }
        std::wstring wide(static_cast<size_t>(wideSize), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), wide.data(), wideSize);
        int ansiSize = WideCharToMultiByte(CP_ACP, 0, wide.data(), wideSize, nullptr, 0, nullptr, nullptr);
        if (ansiSize <= 0) {
            return value;
        }
        std::string ansi(static_cast<size_t>(ansiSize), '\0');
        WideCharToMultiByte(CP_ACP, 0, wide.data(), wideSize, ansi.data(), ansiSize, nullptr, nullptr);
        return ansi;
    }

    void WriteFixedString(std::ofstream& out, const std::string& value, size_t length)
    {
        std::string converted = Utf8ToSystem(value);
        std::vector<char> buffer(length, 0);
      size_t copySize = length < converted.size() ? length : converted.size();
        std::memcpy(buffer.data(), converted.c_str(), copySize);
        out.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    }
}

bool WriteVmdFile(const std::string& path, const MotionClip& clip)
{
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        return false;
    }

    WriteFixedString(out, "Vocaloid Motion Data 0002", 30);
 if (!clip.modelName.empty()) {
        WriteFixedString(out, clip.modelName, 20);
    } else {
        WriteFixedString(out, "AI_Mascot", 20);
    }

    uint32_t boneKeyCount = 0;
    for (const auto& bone : clip.bones) {
        boneKeyCount += static_cast<uint32_t>(bone.frames.size());
    }
    out.write(reinterpret_cast<const char*>(&boneKeyCount), sizeof(boneKeyCount));

    for (const auto& bone : clip.bones) {
        for (const auto& frame : bone.frames) {
            WriteFixedString(out, bone.name, 15);
            uint32_t frameIndex = static_cast<uint32_t>(frame.frame);
            out.write(reinterpret_cast<const char*>(&frameIndex), sizeof(frameIndex));
            out.write(reinterpret_cast<const char*>(frame.pos), sizeof(frame.pos));
            out.write(reinterpret_cast<const char*>(frame.rot), sizeof(frame.rot));
            std::vector<uint8_t> interp(64, 0);
            out.write(reinterpret_cast<const char*>(interp.data()), static_cast<std::streamsize>(interp.size()));
        }
    }

    uint32_t zero = 0;
    out.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
    out.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
    out.write(reinterpret_cast<const char*>(&zero), sizeof(zero));
    return true;
}
