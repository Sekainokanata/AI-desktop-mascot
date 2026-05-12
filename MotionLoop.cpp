#include "MotionLoop.h"

#include "MotionPipeline.h"
#include "CaptureManager.h"
#include "SimpleJson.h"
#include "LlmClient.h"

#include <windows.h>
#include <string>

namespace
{
    bool ParseEvaluationResponse(const std::string& response, bool& ok)
    {
        JsonValue root;
        if (!ParseJson(response, root, nullptr) || !root.IsObject()) {
            return false;
        }
        const JsonValue* choices = root.Find("choices");
        if (choices && choices->IsArray() && !choices->array.empty()) {
            const JsonValue& choice = choices->array.front();
            const JsonValue* message = choice.Find("message");
            if (message && message->IsObject()) {
                const JsonValue* content = message->Find("content");
                if (content && content->IsString()) {
                    JsonValue result;
                    if (ParseJson(content->string, result, nullptr) && result.IsObject()) {
                        const JsonValue* okValue = result.Find("ok");
                        if (okValue && okValue->IsBool()) {
                            ok = okValue->boolean;
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }
}

bool RunMotionFeedbackLoop(const std::string& instruction,
    const std::string& endpointUrl,
    const std::string& modelName,
    const std::string& outputVmdPath,
    const std::string& captureDir,
    int maxIterations,
    int delaySeconds)
{
    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        if (!RunMotionGeneration(instruction, endpointUrl, modelName, outputVmdPath)) {
            return false;
        }
        Sleep(delaySeconds * 1000);
        std::string latestCapture = GetLatestCapturePath(captureDir);
        if (latestCapture.empty()) {
            return false;
        }
        std::string response = RequestMotionEvaluationJson(endpointUrl, modelName, instruction, latestCapture);
        if (response.empty()) {
            return false;
        }
        bool ok = false;
        if (ParseEvaluationResponse(response, ok) && ok) {
            return true;
        }
    }
    return false;
}
