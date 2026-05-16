#pragma once
void attachMotion(int ModelHandle, int animNo, int& AttachIndex, float& TotalTime, float& PlayTime);

// 引数に bool isDebugMode を追加
void Model_animation(float& PlayTime, float TotalTime, int ModelHandle, int AttachIndex, bool isDebugMode);