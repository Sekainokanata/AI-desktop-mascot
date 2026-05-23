#include <stdio.h>
#include "DxLib.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <Windows.h>
#include <shellscalingapi.h>

#include <thread>
#include <mutex>
#include <sstream>

#include "マスコット表示前処理.h"
#include "Animation.h"
#include "MotionPipeline.h"
#include "LlmClient.h"
#include "SimpleJson.h"
#include "CaptureManager.h"
#include "MotionSchema.h" // デバッグ用VMD生成に必要
#include "JsonToVmd.h"    // デバッグ用VMD生成に必要

#pragma comment(lib, "Shcore.lib")

// ==========================================
// デバッグモードの切り替えフラグ
// true: ユーザー手動入力モード, false: LLM自動生成モード
const bool kDebugMode = false;
// ==========================================

const char* kMotionPath = "C:/Users/r-tom/Desktop/AIデスクトップマスコット/Sour式初音ミクVer.1.02/Black000.vmd";
const char* kCaptureDir = "C:/Users/r-tom/Desktop/AIデスクトップマスコット/captures";
const int kMaxFeedbackIterations = 10;
const int kFeedbackDelaySeconds = 3;
const char* kInstruction = "右腕のみを上にあげる";
const char* kEndpointUrl = "http://localhost:1234/v1/chat/completions";
const char* kModelName = "google/gemma-4-e4b";
const char* kModelDir = "C:/Users/r-tom/Desktop/AIデスクトップマスコット/Sour式初音ミクVer.1.02";
char modelPath[] = "C:/Users/r-tom/Desktop/AIデスクトップマスコット/Sour式初音ミクVer.1.02/Black.pmx";

int g_vmdCounter = 0;
std::string g_currentVmdPath = "";

// 次の連番パス（Black000, Black001...）を生成し、古いファイルをフォルダから消去する関数
std::string UpdateVmdPath() {
	if (!g_currentVmdPath.empty()) {
		DeleteFileA(g_currentVmdPath.c_str()); // フォルダ内に古い連番VMDが残らないように削除
	}
	char buf[512];
	sprintf_s(buf, "%s/Black%03d.vmd", kModelDir, g_vmdCounter++);
	g_currentVmdPath = buf;
	return g_currentVmdPath;
}

// --- デバッグ用の入力共有データ ---
struct DebugInputData {
	std::string boneName;
	float rot[4];
	bool hasNewData = false;
};
DebugInputData g_debugData;
std::mutex g_debugMutex;

// Shift-JIS(コンソール入力)をUTF-8に変換する関数
std::string Utf8ToShiftJIS(const std::string& utf8Str) {
	if (utf8Str.empty()) return "";
	int wideSize = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
	std::wstring wide(wideSize, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wide[0], wideSize);

	int sjisSize = WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string sjis(sjisSize, '\0');
	WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, &sjis[0], sjisSize, nullptr, nullptr);

	// 終端のNULL文字を取り除く
	sjis.resize(strlen(sjis.c_str()));
	return sjis;
}
// デバッグ用入力スレッド
void DebugInputThread() {
	while (true) {
		std::string boneNameSjis;
		std::cout << "\n[Debug] 動かしたいボーン名を入力してください (例: 右腕): ";
		if (!(std::cin >> boneNameSjis)) break;

		std::string rotStr;
		std::cout << "[Debug] 回転(x,y,z,w)をカンマ区切りで入力 (例: 0,0,0,1): ";
		if (!(std::cin >> rotStr)) break;

		// カンマをスペースに置換してパースしやすくする
		for (char& c : rotStr) {
			if (c == ',') c = ' ';
		}
		float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;
		std::stringstream ss(rotStr);
		ss >> x >> y >> z >> w;



		// スレッドセーフにデータをメインスレッドへ渡す
		std::lock_guard<std::mutex> lock(g_debugMutex);
		g_debugData.boneName = boneNameSjis; // ★ Shift-JISのまま渡す！
		g_debugData.rot[0] = x;
		g_debugData.rot[1] = y;
		g_debugData.rot[2] = z;
		g_debugData.rot[3] = w;
		g_debugData.hasNewData = true;
	}
}

// デバッグ用のVMDファイル直接生成関数
void CreateDebugVmd(const std::string& vmdPath, const std::string& boneName, const float rot[4]) {
	MotionClip clip;
	clip.fps = 30;
	clip.duration = 30;
	clip.modelName = "Sour_Miku_Black";

	MotionBone bone;
	bone.name = boneName;

	// 0フレーム目と30フレーム目に同じ回転を登録（静止ポーズ）
	MotionFrame frame0;
	frame0.frame = 0;
	frame0.rot[0] = rot[0]; frame0.rot[1] = rot[1]; frame0.rot[2] = rot[2]; frame0.rot[3] = rot[3];
	bone.frames.push_back(frame0);

	MotionFrame frame30;
	frame30.frame = 30;
	frame30.rot[0] = rot[0]; frame30.rot[1] = rot[1]; frame30.rot[2] = rot[2]; frame30.rot[3] = rot[3];
	bone.frames.push_back(frame30);

	clip.bones.push_back(bone);
	WriteVmdFile(vmdPath, clip);
	printf("\n[Debug] ボーン [%s] のモーションを VMD に書き出しました。\n", boneName.c_str());
}
// ------------------------------------------

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
	char modelPath[] = "C:/Users/r-tom/Desktop/AIデスクトップマスコット/Sour式初音ミクVer.1.02/Black.pmx";
	const char* capturePathFormat = "C:/Users/r-tom/Desktop/AIデスクトップマスコット/captures/mascot_%05d.jpg";
	CreateDirectoryA(kCaptureDir, nullptr);

	// ★ PMXはここで1回だけロードし、絶対にループ内で消さない
	int ModelHandle = MV1LoadModel(modelPath);
	if (ModelHandle == -1) return;

	// ★ モデルの初期設定（位置やスケール）もここで1回だけ行う
	MV1SetScale(ModelHandle, VGet(40.0f, 40.0f, 40.0f));
	MV1SetPosition(ModelHandle, VGet(width - 300.0f, 3.0f, 0.0f));
	MV1SetRotationXYZ(ModelHandle, VGet(0.0f, 0.6f, 0.0f));

	int materialNum = MV1GetMaterialNum(ModelHandle);
	for (int i = 0; i < materialNum; i++)
	{
		MV1SetMaterialOutLineDotWidth(ModelHandle, i, 0.001f);
	}

	int AttachIndex = -1;
	int VmdHandle = -1;   // ロードしたVMDデータを管理するための変数
	float TotalTime = 0.0f;
	float PlayTime = 0.0f;
	int captureFrameCount = 0;
	int captureIndex = 0;
	int feedbackIteration = 0;
	std::string lastCapturePath;
	float previousPlayTime = 0.0f;

	std::vector<std::string> feedbackHistory;
	std::string lastGeneratedJson;

	// === デバッグモード分岐 ===
	if (kDebugMode) {
		printf("=========================================\n");
		printf(" Debug Mode ON\n");
		printf(" コンソールからボーン名と回転を入力できます。\n");
		printf("=========================================\n");
		std::thread inputThread(DebugInputThread);
		inputThread.detach();

		// 最初はVMDがないため、アタッチはせず棒立ち状態（静止）にしておく
		AttachIndex = -1;
		VmdHandle = -1;
		TotalTime = 0.0f;
		PlayTime = 0.0f;
	}
	else {
		// ★ 修正: GetCurrentMotionPath() ではなく UpdateVmdPath() を使う
		std::string activePath = UpdateVmdPath();
		if (!RunMotionGeneration(kInstruction, kEndpointUrl, kModelName, activePath.c_str(), feedbackHistory, lastGeneratedJson)) {
			MV1DeleteModel(ModelHandle);
			return;
		}
		// モデルハンドルは壊さず、VMDだけをアタッチする
		attachMotion(ModelHandle, activePath.c_str(), VmdHandle, AttachIndex, TotalTime, PlayTime);
	}

	while (ProcessMessage() == 0)
	{
		// === デバッグモード: 新しい入力があればVMDを更新 ===
		if (kDebugMode) {
			bool hasNew = false;
			std::string bName;
			float bRot[4];
			{
				std::lock_guard<std::mutex> lock(g_debugMutex);
				if (g_debugData.hasNewData) {
					bName = g_debugData.boneName;
					for (int i = 0; i < 4; ++i) bRot[i] = g_debugData.rot[i];
					g_debugData.hasNewData = false;
					hasNew = true;
				}
			}

			// ★ 修正: 抜け落ちていた「入力された時の反映処理」を追加
			if (hasNew) {
				std::string activePath = UpdateVmdPath(); // 次の連番パスを取得
				CreateDebugVmd(activePath, bName, bRot);
				// PMXはそのままに、新しい連番VMDだけをアタッチ
				attachMotion(ModelHandle, activePath.c_str(), VmdHandle, AttachIndex, TotalTime, PlayTime);
			}
		}

		ClearDrawScreen();

		previousPlayTime = PlayTime;
		Model_animation(PlayTime, TotalTime, ModelHandle, AttachIndex, kDebugMode);

		// ロードが成功している場合のみ描画
		if (ModelHandle != -1) {
			MV1DrawModel(ModelHandle);
		}

		if (captureFrameCount % 30 == 0) {
			char capturePath[MAX_PATH] = {};
			sprintf_s(capturePath, capturePathFormat, captureIndex);

			int cropX1 = width - 600;
			int cropY1 = 0;
			int cropX2 = width;
			int cropY2 = height;

			if (cropX1 < 0) cropX1 = 0;
			if (cropY1 < 0) cropY1 = 0;

			SaveDrawScreenToJPEG(cropX1, cropY1, cropX2, cropY2, capturePath, 80);

			lastCapturePath = capturePath;
			captureIndex++;
		}
		captureFrameCount++;

		if (CheckHitKey(KEY_INPUT_Q)) {
			break;
		}

		// === AIフィードバックループ（デバッグモード時はスキップ） ===
		if (!kDebugMode) {
			if (PlayTime < previousPlayTime && feedbackIteration < kMaxFeedbackIterations) {
				std::vector<std::string> capturePaths = GetAllCapturePaths(kCaptureDir);

				if (!capturePaths.empty()) {
					std::string response = RequestMotionEvaluationJson(kEndpointUrl, kModelName, kInstruction, capturePaths, lastGeneratedJson);
					DeleteCaptures(capturePaths);
					captureFrameCount = 0;
					captureIndex = 0;

					bool ok = false;
					std::string detectedMovement;
					std::string advice;

					if (ParseEvaluationResponse(response, ok, detectedMovement, advice) && ok) {
						feedbackIteration = kMaxFeedbackIterations;
					}
					else {
						std::string historySummary = "Generated parameters resulted in: '" + detectedMovement + "'. Advice for correction: " + advice;
						feedbackHistory.push_back(historySummary);
						printf("[Feedback] Added history: %s\n", historySummary.c_str());

						feedbackIteration++;
						Sleep(kFeedbackDelaySeconds * 1000);

						// 再生成のタイミングで連番を更新し、新しいパスでAIに作成させる
						std::string activePath = UpdateVmdPath();
						if (!RunMotionGeneration(kInstruction, kEndpointUrl, kModelName, activePath.c_str(), feedbackHistory, lastGeneratedJson)) {
							break;
						}
						// ★ 修正: 旧式の attachMotion 呼び出しを、VMDスワップ式の呼び出しに変更
						attachMotion(ModelHandle, activePath.c_str(), VmdHandle, AttachIndex, TotalTime, PlayTime);
					}
				}
			}
		}
	}

	// ★ 修正: 終了時にVmdHandleも忘れずに解放する
	if (VmdHandle != -1) MV1DeleteModel(VmdHandle);
	if (ModelHandle != -1) MV1DeleteModel(ModelHandle);
}


int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	AllocConsole();
	FILE* pFile = nullptr;
	freopen_s(&pFile, "CONOUT$", "w", stdout);
	freopen_s(&pFile, "CONOUT$", "w", stderr);

	// 追加: コンソールの入力を受け付けるために stdin を割り当てる
	freopen_s(&pFile, "CONIN$", "r", stdin);

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