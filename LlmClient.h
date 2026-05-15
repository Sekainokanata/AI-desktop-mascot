#pragma once

#include <string>
#include <vector>

std::string RequestMotionJson(const std::string& endpointUrl, const std::string& modelName, const std::string& prompt);

// 第4引数を std::vector<std::string> に変更
std::string RequestMotionEvaluationJson(const std::string& endpointUrl, const std::string& modelName, const std::string& instruction, const std::vector<std::string>& imagePaths, const std::string& lastGeneratedJson);