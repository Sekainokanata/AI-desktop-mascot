#include "CaptureManager.h"
#include <windows.h>
#include <algorithm>

std::vector<std::string> GetAllCapturePaths(const std::string& captureDir)
{
    std::vector<std::string> paths;
    std::string pattern = captureDir + "\\*.jpg";
    WIN32_FIND_DATAA data = {};
    HANDLE handle = FindFirstFileA(pattern.c_str(), &data);

    if (handle == INVALID_HANDLE_VALUE) {
        return paths;
    }
    do {
        // ディレクトリでないファイルのみ追加
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            paths.push_back(captureDir + "\\" + data.cFileName);
        }
    } while (FindNextFileA(handle, &data));
    FindClose(handle);

    // mascot_00000.jpg のように連番なので、名前順でソートして時系列を保つ
    std::sort(paths.begin(), paths.end());
    return paths;
}

void DeleteCaptures(const std::vector<std::string>& paths)
{
    for (const auto& path : paths) {
        DeleteFileA(path.c_str());
    }
}