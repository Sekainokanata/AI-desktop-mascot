#include "MotionLoop.h"

#include "MotionPipeline.h"
#include "CaptureManager.h"
#include "SimpleJson.h"
#include "LlmClient.h"

#include <windows.h>
#include <string>
#include <vector> 

namespace
{
    // 内部のパース関数も、分析結果とアドバイスを受け取れるように拡張します
    bool ParseEvaluationResponse(const std::string& response, bool& ok, std::string& detectedMovement, std::string& advice)
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
                        }
                        const JsonValue* movementValue = result.Find("detected_movement");
                        if (movementValue && movementValue->IsString()) {
                            detectedMovement = movementValue->string;
                        }
                        const JsonValue* adviceValue = result.Find("advice");
                        if (adviceValue && adviceValue->IsString()) {
                            advice = adviceValue->string;
                        }
                        return true;
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
    // --- 追加：ループ内で共有する履歴と直前JSONデータ ---
    std::vector<std::string> history;
    std::string lastGeneratedJson;

    for (int iteration = 0; iteration < maxIterations; ++iteration) {
        // 新しい引数構成（history, lastGeneratedJson）を指定して呼び出し
        if (!RunMotionGeneration(instruction, endpointUrl, modelName, outputVmdPath, history, lastGeneratedJson)) {
            return false;
        }
        Sleep(delaySeconds * 1000);

        std::vector<std::string> capturePaths = GetAllCapturePaths(captureDir);
        if (capturePaths.empty()) {
            return false;
        }

        // 評価リクエスト（配列をそのまま渡す）
        std::string response = RequestMotionEvaluationJson(endpointUrl, modelName, instruction, capturePaths, lastGeneratedJson);

        // 評価し終わった画像群はここで削除しておく
        DeleteCaptures(capturePaths);

        bool ok = false;
        std::string detectedMovement;
        std::string advice;

        if (ParseEvaluationResponse(response, ok, detectedMovement, advice) && ok) {
            return true; // 成功したらループを抜ける
        }

        // 失敗した場合は履歴を追加して次の試行へ活かす
        std::string attemptSummary = "Attempt " + std::to_string(iteration + 1) + " resulted in: '" + detectedMovement + "'. Advice: " + advice;
        history.push_back(attemptSummary);
    }
    return false;
}