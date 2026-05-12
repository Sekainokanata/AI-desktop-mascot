#pragma once

#include <string>

bool RunMotionGeneration(const std::string& instruction,
    const std::string& endpointUrl,
    const std::string& modelName,
    const std::string& outputVmdPath);
