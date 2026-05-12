#include "MotionPipeline.h"

#include "PromptBuilder.h"
#include "LlmClient.h"
#include "MotionSchema.h"
#include "JsonToVmd.h"

#include <string>
#include <vector>
#include <windows.h>

#include "SimpleJson.h"

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

    void LogText(const char* label, const std::string& text)
    {
        printf("[%s] size=%zu\n", label, text.size());
        printf("[%s] begin\n", label);
       std::string output = Utf8ToSystem(text);
        printf("%s\n", output.c_str());
        printf("[%s] end\n", label);
    }

    std::string ExtractJsonFromResponse(const std::string& response)
    {
        JsonValue root;
        if (!ParseJson(response, root, nullptr) || !root.IsObject()) {
          printf("[MotionPipeline] Response JSON parse failed.\n");
            return {};
        }

        const JsonValue* choices = root.Find("choices");
        if (!choices || !choices->IsArray() || choices->array.empty()) {
          printf("[MotionPipeline] Response missing choices array.\n");
            return {};
        }

        const JsonValue& choice = choices->array.front();
        if (!choice.IsObject()) {
          printf("[MotionPipeline] Response choice is not object.\n");
            return {};
        }

        const JsonValue* message = choice.Find("message");
        if (!message || !message->IsObject()) {
          printf("[MotionPipeline] Response missing message object.\n");
            return {};
        }

        const JsonValue* content = message->Find("content");
        if (!content || !content->IsString()) {
          printf("[MotionPipeline] Response missing content string.\n");
            return {};
        }

        const std::string& text = content->string;
       LogText("LLM_CONTENT", text);
        size_t blockStart = text.find("```json");
        if (blockStart != std::string::npos) {
            blockStart = text.find('\n', blockStart);
            if (blockStart == std::string::npos) {
              printf("[MotionPipeline] JSON block missing newline.\n");
                return {};
            }
            ++blockStart;
            size_t blockEnd = text.find("```", blockStart);
            if (blockEnd == std::string::npos || blockEnd <= blockStart) {
              printf("[MotionPipeline] JSON block end not found.\n");
                return {};
            }
            return text.substr(blockStart, blockEnd - blockStart);
        }

        size_t start = text.find('{');
        size_t end = text.rfind('}');
        if (start == std::string::npos || end == std::string::npos || end <= start) {
          printf("[MotionPipeline] JSON braces not found in content.\n");
            return {};
        }
        return text.substr(start, end - start + 1);
    }
}

bool RunMotionGeneration(const std::string& instruction,
    const std::string& endpointUrl,
    const std::string& modelName,
    const std::string& outputVmdPath)
{
    std::vector<std::string> boneNames = GetDefaultBoneNames();
    if (boneNames.empty()) {
        return false;
    }

    std::string prompt = BuildMotionPrompt(instruction, boneNames);
   LogText("LLM_PROMPT", prompt);
    std::string response = RequestMotionJson(endpointUrl, modelName, prompt);
    if (response.empty()) {
       printf("[MotionPipeline] Empty response from LLM.\n");
        return false;
    }
    LogText("LLM_RESPONSE", response);

    std::string json = ExtractJsonFromResponse(response);
    if (json.empty()) {
       printf("[MotionPipeline] Motion JSON extraction failed.\n");
        return false;
    }
    LogText("MOTION_JSON", json);

    MotionClip clip;
    if (!ParseMotionJson(json, clip)) {
       printf("[MotionPipeline] Motion JSON parse failed.\n");
        return false;
    }
  ExpandMissingFrames(clip, boneNames);
    printf("[MotionPipeline] Parsed bones: %zu\n", clip.bones.size());

   bool written = WriteVmdFile(outputVmdPath, clip);
    printf("[MotionPipeline] VMD write %s: %s\n", written ? "succeeded" : "failed", outputVmdPath.c_str());
    return written;
}
