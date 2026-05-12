#pragma once

#include <string>
#include <vector>

std::string BuildMotionPrompt(const std::string& instruction, const std::vector<std::string>& boneNames, const std::vector<std::string>& history);