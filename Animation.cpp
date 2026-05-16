#include <stdio.h>
#include "DxLib.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <Windows.h>
#include <shellscalingapi.h>
#include "Animation.h"

#pragma comment(lib, "Shcore.lib")

// 第3引数を int& VmdHandle に変更
void attachMotion(int ModelHandle, const char* vmdPath, int& VmdHandle, int& AttachIndex, float& TotalTime, float& PlayTime)
{
	// 1. 既にアタッチされている場合は安全にデタッチ
	if (AttachIndex != -1) {
		MV1DetachAnim(ModelHandle, AttachIndex);
		AttachIndex = -1;
	}

	// 2. ★修正: 以前にロードしたVMDデータ(モデル扱い)をメモリから完全に解放
	if (VmdHandle != -1) {
		MV1DeleteModel(VmdHandle);
		VmdHandle = -1;
	}

	// 3. ★修正: ディスク上の最新VMDファイルを「モデルとして」新しく読み込む
	VmdHandle = MV1LoadModel(vmdPath);
	if (VmdHandle == -1) {
		printf("Error: アニメーション(VMD)の読み込みに失敗しました: %s\n", vmdPath);
		return;
	}

	// 4. ★修正: MV1AttachAnim の第3引数に VmdHandle を渡してアタッチする
	// MV1AttachAnim( アタッチ先モデル, アニメ番号0, VMDのハンドル )
	AttachIndex = MV1AttachAnim(ModelHandle, 0, VmdHandle);
	if (AttachIndex == -1) {
		printf("Error: アニメーションのアタッチに失敗しました\n");
		return;
	}

	MV1SetAttachAnimBlendRate(ModelHandle, AttachIndex, 1.0f);
	TotalTime = MV1GetAttachAnimTotalTime(ModelHandle, AttachIndex);
	PlayTime = 0.0f;
}

// （Model_animation 関数は変更なし）
void Model_animation(float& PlayTime, float TotalTime, int ModelHandle, int AttachIndex, bool isDebugMode)
{
	if (AttachIndex == -1) return;

	PlayTime += 0.5f;

	if (PlayTime > TotalTime) {
		PlayTime = 0.0f;
	}

	if (!isDebugMode) {
		static int frameCount = 0;
		if (frameCount % 15 == 0) {
			printf("PlayTime: %f / TotalTime: %f, AttachIndex: %d\n", PlayTime, TotalTime, AttachIndex);
		}
		frameCount++;
	}

	MV1SetAttachAnimTime(ModelHandle, AttachIndex, PlayTime);
}