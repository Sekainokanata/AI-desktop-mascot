#pragma once
#include <string>
#include <vector>

std::string GetLatestCapturePath(const std::string& captureDir);

std::vector<std::string> GetAllCapturePaths(const std::string& captureDir);
void DeleteCaptures(const std::vector<std::string>& paths);