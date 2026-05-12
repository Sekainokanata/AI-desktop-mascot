#pragma once

#include <string>
#include <vector>

bool RunMotionGeneration(const std::string& instruction,
    const std::string& endpointUrl,
    const std::string& modelName,
    const std::string& outputVmdPath,
    const std::vector<std::string>& history,
    std::string& outGeneratedJson);
