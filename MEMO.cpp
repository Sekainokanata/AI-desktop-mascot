/*#include <stdio.h>
#include "DxLib.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <Windows.h>
#include <shellscalingapi.h>

#include "マスコット表示前処理.h"

#pragma comment(lib, "Shcore.lib")

// 引数に `&` を付けることで「参照渡し」となり、呼び出し元の変数を直接書き換えられます
void move_settings(int ModelHandle, int& AttachIndex, float& TotalTime)
{
	// 自動読み込みされた「アニメーション番号 0」を、自分自身（-1）からアタッチする
	// ※自動結合ならボーン構造が完全に一致しているため、確実に対応します
	AttachIndex = MV1AttachAnim(ModelHandle, 0, -1, TRUE);

	if (AttachIndex == -1) {
		printf("Error: アニメーションのアタッチに失敗しました\n");
		return;
	}

	TotalTime = MV1GetAttachAnimTotalTime(ModelHandle, AttachIndex);
	printf("Animation loaded: TotalTime = %f\n", TotalTime);
}

// PlayTime を更新し続ける必要があるため、ここでも PlayTime に `&` を付けます
void Model_animation(float& PlayTime, float TotalTime, int ModelHandle, int AttachIndex)
{
	PlayTime += 0.5f;

	if (PlayTime > TotalTime) {
		PlayTime = 0.0f;
	}

	// デバッグ出力（数フレームごとに）
	static int frameCount = 0;
	if (frameCount % 60 == 0) {
		printf("PlayTime: %f / TotalTime: %f, AttachIndex: %d\n", PlayTime, TotalTime, AttachIndex);
	}
	frameCount++;

	MV1SetAttachAnimTime(ModelHandle, AttachIndex, PlayTime);
}

void mainsystem(int width, int height)
{
	// MMDモデルを読み込む
	// ※同じフォルダに「Black000.vmd」があるため、この1行だけでモーションも完璧にロードされます
	char modelPath[] = "C:/Users/r-tom/Desktop/AIデスクトップマスコット/Sour式初音ミクVer.1.02/Black.pmx";

	int ModelHandle = MV1LoadModel(modelPath);

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

	auto attachMotion = [&](int animNo) {
		if (AttachIndex != -1) {
			MV1DetachAnim(ModelHandle, AttachIndex);
		}
		AttachIndex = MV1AttachAnim(ModelHandle, animNo, -1, TRUE);
		if (AttachIndex != -1) {
			MV1SetAttachAnimBlendRate(ModelHandle, AttachIndex, 1.0f);
			TotalTime = MV1GetAttachAnimTotalTime(ModelHandle, AttachIndex);
			PlayTime = 0.0f;
		}
		};

	attachMotion(0);

	while (ProcessMessage() == 0)
	{
		if (CheckHitKey(KEY_INPUT_1)) { currentAnim = 0; attachMotion(0); }
		if (CheckHitKey(KEY_INPUT_2)) { currentAnim = 1; attachMotion(1); }
		if (CheckHitKey(KEY_INPUT_3)) { currentAnim = 2; attachMotion(2); }

		ClearDrawScreen();
		Model_animation(PlayTime, TotalTime, ModelHandle, AttachIndex);
		MV1DrawModel(ModelHandle);
		ScreenFlip();
		if (CheckHitKey(KEY_INPUT_Q)) {
			break;
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
}*/