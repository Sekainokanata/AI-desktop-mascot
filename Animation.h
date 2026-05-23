#pragma once

// 引数をモデルのリロード用に変更（ModelHandle を参照渡し int& にするのがポイントです）
void attachMotion(int ModelHandle, const char* vmdPath, int& VmdHandle, int& AttachIndex, float& TotalTime, float& PlayTime);

void Model_animation(float& PlayTime, float TotalTime, int ModelHandle, int AttachIndex, bool isDebugMode);