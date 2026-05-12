#include <stdio.h>
#include "DxLib.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <Windows.h>
#include <shellscalingapi.h>

#include "マスコット表示前処理.h"
#include "Animation.h"
#include "MotionPipeline.h"
#include "LlmClient.h"
#include "SimpleJson.h"

#pragma comment(lib, "Shcore.lib")

const char* kMotionPath = "C:/Users/r-tom/Desktop/AIデスクトップマスコット/Sour式初音ミクVer.1.02/Black000.vmd";
const char* kCaptureDir = "C:/Users/r-tom/Desktop/AIデスクトップマスコット/captures";
const int kMaxFeedbackIterations = 10;
const int kFeedbackDelaySeconds = 3;
const char* kInstruction = "手を振る";
const char* kEndpointUrl = "http://localhost:1234/v1/chat/completions";
const char* kModelName = "google/gemma-4-e4b";


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
					// --- 追加：分析文面とアドバイスを抽出 ---
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


bool tryGetFileWriteTime(const char* path, ULONGLONG& writeTime)
{
	WIN32_FILE_ATTRIBUTE_DATA data;
	if (!GetFileAttributesExA(path, GetFileExInfoStandard, &data)) {
		return false;
	}
	writeTime = (static_cast<ULONGLONG>(data.ftLastWriteTime.dwHighDateTime) << 32) | data.ftLastWriteTime.dwLowDateTime;
	return true;
}



void mainsystem(int width, int height)
{
  // MMDモデルを読み込む
	// ※同じフォルダに「Black000L.vmd」があるため、この1行だけでモーションも完璧にロードされます
	char modelPath[] = "C:/Users/r-tom/Desktop/AIデスクトップマスコット/Sour式初音ミクVer.1.02/Black.pmx";
	const char* capturePathFormat = "C:/Users/r-tom/Desktop/AIデスクトップマスコット/captures/mascot_%05d.jpg";
	CreateDirectoryA(kCaptureDir, nullptr);

	int ModelHandle = MV1LoadModel(modelPath);
	if (ModelHandle == -1) {
		return;
	}

	MV1SetScale(ModelHandle, VGet(40.0f, 40.0f, 40.0f));
	MV1SetPosition(ModelHandle, VGet(width - 300.0f, 3.0f, 0.0f));

	// モデルをY軸回転させて正面を向かせる
	MV1SetRotationXYZ(ModelHandle, VGet(0.0f, 0.6f, 0.0f));

	// 外線（エッジ）の太さを細くする
	int materialNum = MV1GetMaterialNum(ModelHandle);
	for (int i = 0; i < materialNum; i++)
	{
		MV1SetMaterialOutLineDotWidth(ModelHandle, i, 0.001f);
	}

	int currentAnim = 0;
	int AttachIndex = -1;
	float TotalTime = 0.0f;
	float PlayTime = 0.0f;
	int captureFrameCount = 0;
	int captureIndex = 0;
	int feedbackIteration = 0;
	std::string lastCapturePath;
	float previousPlayTime = 0.0f;

	// --- 追加：履歴保存用ベクターと直前のJSON保存用文字列 ---
	std::vector<std::string> feedbackHistory;
	std::string lastGeneratedJson;

	// 初回呼び出し時の引数変更
	if (!RunMotionGeneration(kInstruction, kEndpointUrl, kModelName, kMotionPath, feedbackHistory, lastGeneratedJson)) {
		MV1DeleteModel(ModelHandle);
		return;
	}
	attachMotion(ModelHandle, currentAnim, AttachIndex, TotalTime, PlayTime);

   const char* vmdPath = kMotionPath;
	ULONGLONG lastWriteTime = 0;
	bool hasWriteTime = tryGetFileWriteTime(vmdPath, lastWriteTime);

	while (ProcessMessage() == 0)
	{
        ULONGLONG currentWriteTime = 0;
		if (tryGetFileWriteTime(vmdPath, currentWriteTime)) {
			if (!hasWriteTime || currentWriteTime != lastWriteTime) {
				lastWriteTime = currentWriteTime;
				hasWriteTime = true;
				attachMotion(ModelHandle, currentAnim, AttachIndex, TotalTime, PlayTime);
			}
		}

      ClearDrawScreen();

		previousPlayTime = PlayTime;
		Model_animation(PlayTime, TotalTime, ModelHandle, AttachIndex);

		MV1DrawModel(ModelHandle);

		/*if (captureFrameCount % 30 == 0) {
			char capturePath[MAX_PATH] = {};
			sprintf_s(capturePath, capturePathFormat, captureIndex);
			SaveDrawScreen(0, 0, width, height, capturePath);
			lastCapturePath = capturePath;
			captureIndex++;
		}*/
		if (captureFrameCount % 30 == 0) {
			char capturePath[MAX_PATH] = {};
			sprintf_s(capturePath, capturePathFormat, captureIndex);

			int cropX1 = width - 600;
			int cropY1 = 0;
			int cropX2 = width;
			int cropY2 = height;

			if (cropX1 < 0) cropX1 = 0;
			if (cropY1 < 0) cropY1 = 0;

			// --- 修正：確実にJPEG圧縮して保存する ---
			// 最後の引数(品質)は 80 前後を指定してファイルサイズを抑えます
			SaveDrawScreenToJPEG(cropX1, cropY1, cropX2, cropY2, capturePath, 80);

			lastCapturePath = capturePath;
			captureIndex++;
		}
		captureFrameCount++;

		ScreenFlip();

		if (CheckHitKey(KEY_INPUT_Q)) {
			break;
		}

		if (PlayTime < previousPlayTime && feedbackIteration < kMaxFeedbackIterations && !lastCapturePath.empty()) {
			// 評価リクエスト送信時に直前のJSON情報を付与
			std::string response = RequestMotionEvaluationJson(kEndpointUrl, kModelName, kInstruction, lastCapturePath, lastGeneratedJson);

			bool ok = false;
			std::string detectedMovement;
			std::string advice;

			if (ParseEvaluationResponse(response, ok, detectedMovement, advice) && ok) {
				feedbackIteration = kMaxFeedbackIterations; // 合格ならループ完了
			}
			else {
				// --- 追加：不合格だった場合、動きの分析を履歴文字列としてストック ---
				std::string historySummary = "Generated parameters resulted in: '" + detectedMovement + "'. Advice for correction: " + advice;
				feedbackHistory.push_back(historySummary);
				printf("[Feedback] Added history: %s\n", historySummary.c_str());

				feedbackIteration++;
				Sleep(kFeedbackDelaySeconds * 1000);

				// 次回生成時に蓄積された feedbackHistory を渡す
				if (!RunMotionGeneration(kInstruction, kEndpointUrl, kModelName, kMotionPath, feedbackHistory, lastGeneratedJson)) {
					break;
				}
				attachMotion(ModelHandle, currentAnim, AttachIndex, TotalTime, PlayTime);
				captureFrameCount = 0;
				captureIndex = 0;
				lastCapturePath.clear();
			}
		}
	}

	MV1DeleteModel(ModelHandle);
}


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	// コンソール有効化
	AllocConsole();
	FILE* pFile = nullptr;
	freopen_s(&pFile, "CONOUT$", "w", stdout);
	freopen_s(&pFile, "CONOUT$", "w", stderr);

	int width = 0;
	int height = 0;
	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
	preInitialize(&width, &height);
	if (DxLib_Init() == -1)
		return -1;
	afterInitialize();
	mainsystem(width, height);
	DxLib_End();
	return 0;
}