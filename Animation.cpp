#include <stdio.h>
#include "DxLib.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <Windows.h>
#include <shellscalingapi.h>

#pragma comment(lib, "Shcore.lib")

void attachMotion(int ModelHandle, int animNo, int& AttachIndex, float& TotalTime, float& PlayTime)
{
	if (AttachIndex != -1) {
		MV1DetachAnim(ModelHandle, AttachIndex);
	}
	AttachIndex = MV1AttachAnim(ModelHandle, animNo, -1, TRUE);
	if (AttachIndex == -1) {
		printf("Error: アニメーションのアタッチに失敗しました\n");
		return;
	}
	MV1SetAttachAnimBlendRate(ModelHandle, AttachIndex, 1.0f);
	TotalTime = MV1GetAttachAnimTotalTime(ModelHandle, AttachIndex);
	PlayTime = 0.0f;
}

// 引数 isDebugMode を追加
void Model_animation(float& PlayTime, float TotalTime, int ModelHandle, int AttachIndex, bool isDebugMode)
{
	// アニメーションがアタッチされていない（初期状態）なら処理を抜ける
	if (AttachIndex == -1) return;

	PlayTime += 0.5f;

	if (PlayTime > TotalTime) {
		PlayTime = 0.0f;
	}

	// デバッグモードでない時のみログを出力する
	if (!isDebugMode) {
		static int frameCount = 0;
		if (frameCount % 15 == 0) {
			printf("PlayTime: %f / TotalTime: %f, AttachIndex: %d\n", PlayTime, TotalTime, AttachIndex);
		}
		frameCount++;
	}

	MV1SetAttachAnimTime(ModelHandle, AttachIndex, PlayTime);
}