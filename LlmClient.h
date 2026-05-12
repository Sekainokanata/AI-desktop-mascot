#pragma once

#include <string>

std::string RequestMotionJson(const std::string& endpointUrl, const std::string& modelName, const std::string& prompt);

std::string RequestMotionEvaluationJson(const std::string& endpointUrl, const std::string& modelName, const std::string& instruction, const std::string& imagePath);
