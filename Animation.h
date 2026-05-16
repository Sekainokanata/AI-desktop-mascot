#pragma once

// ★ 第3引数 を int& VmdHandle に変更
void attachMotion(int ModelHandle, const char* vmdPath, int& VmdHandle, int& AttachIndex, float& TotalTime, float& PlayTime);

void Model_animation(float& PlayTime, float TotalTime, int ModelHandle, int AttachIndex, bool isDebugMode);