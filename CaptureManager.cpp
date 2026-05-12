#include "CaptureManager.h"

#include <windows.h>

std::string GetLatestCapturePath(const std::string& captureDir)
{
    std::string pattern = captureDir + "\\*.jpg";
    WIN32_FIND_DATAA data = {};
    HANDLE handle = FindFirstFileA(pattern.c_str(), &data);
    if (handle == INVALID_HANDLE_VALUE) {
        return {};
    }
    std::string latestPath;
    FILETIME latestTime = {};
    bool hasLatest = false;
    do {
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }
        if (!hasLatest || CompareFileTime(&data.ftLastWriteTime, &latestTime) > 0) {
            latestTime = data.ftLastWriteTime;
            hasLatest = true;
            latestPath = captureDir + "\\" + data.cFileName;
        }
    } while (FindNextFileA(handle, &data));
    FindClose(handle);
    return latestPath;
}
