#include <stdio.h>
#include "DxLib.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <Windows.h>
#include <shellscalingapi.h>

#pragma comment(lib, "Shcore.lib")


void preInitialize(int* width, int* height)
{
	RECT rc;
	GetWindowRect(GetDesktopWindow(), &rc);
	*width = rc.right - rc.left;
	*height = rc.bottom - rc.top;
	printf("Desktop resolution: %d x %d\n", *width, *height);
	SetGraphMode(*width, *height, 32);
	ChangeWindowMode(TRUE);
	SetWindowStyleMode(2);
	SetUseBackBufferTransColorFlag(TRUE);
}




void afterInitialize() {
	HWND hWnd = GetMainWindowHandle();
	SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	SetAlwaysRunFlag(TRUE);
	SetDrawScreen(DX_SCREEN_BACK);
}