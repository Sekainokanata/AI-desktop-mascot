#include <stdio.h>
#include "DxLib.h"
#include "Animation.h"

// 引数を VmdHandle に戻す
void attachMotion(int ModelHandle, const char* vmdPath, int& VmdHandle, int& AttachIndex, float& TotalTime, float& PlayTime)
{
	// 古いアニメーションをモデルからデタッチ
	if (AttachIndex != -1) {
		MV1DetachAnim(ModelHandle, AttachIndex);
		AttachIndex = -1;
	}

	// 古いVMDデータ（モーション）だけをメモリから削除
	if (VmdHandle != -1) {
		MV1DeleteModel(VmdHandle);
		VmdHandle = -1;
	}

	// 新しいVMDデータだけをロード
	VmdHandle = MV1LoadModel(vmdPath);
	if (VmdHandle == -1) {
		printf("Error: VMDのロードに失敗しました: %s\n", vmdPath);
		return;
	}

	// 第3引数に VmdHandle を指定してアタッチ（※ボーン名がShift-JISで一致していれば必ず成功します）
	AttachIndex = MV1AttachAnim(ModelHandle, 0, VmdHandle);
	if (AttachIndex != -1) {
		MV1SetAttachAnimBlendRate(ModelHandle, AttachIndex, 1.0f);
		TotalTime = MV1GetAttachAnimTotalTime(ModelHandle, AttachIndex);
	}
	PlayTime = 0.0f;
}

// （Model_animation 関数はそのまま変更なし）
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