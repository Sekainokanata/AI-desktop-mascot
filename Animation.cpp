#include <stdio.h>
#include "DxLib.h"
#include "Animation.h"

void attachMotion(int ModelHandle, const char* vmdPath, int& VmdHandle, int& AttachIndex, float& TotalTime, float& PlayTime)
{
	// 1. 既にアタッチされている場合は安全にデタッチ
	if (AttachIndex != -1) {
		MV1DetachAnim(ModelHandle, AttachIndex);
		AttachIndex = -1;
	}

	// 2. 以前にロードしたVMDデータ（DxLibではモデル扱い）をメモリから完全に解放
	if (VmdHandle != -1) {
		MV1DeleteModel(VmdHandle);
		VmdHandle = -1;
	}

	// 3. ディスク上の最新VMDファイルを新しく読み込む
	VmdHandle = MV1LoadModel(vmdPath);
	if (VmdHandle == -1) {
		printf("Error: アニメーション(VMD)の読み込みに失敗しました: %s\n", vmdPath);
		return;
	}

	// 4. MV1AttachAnim の第3引数に VmdHandle を渡してアタッチする
	AttachIndex = MV1AttachAnim(ModelHandle, 0, VmdHandle);
	if (AttachIndex == -1) {
		printf("Error: アニメーションのアタッチに失敗しました\n");
		return;
	}

	MV1SetAttachAnimBlendRate(ModelHandle, AttachIndex, 1.0f);
	TotalTime = MV1GetAttachAnimTotalTime(ModelHandle, AttachIndex);
	PlayTime = 0.0f;
}

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