#pragma once

#include <string>

bool RunMotionFeedbackLoop(const std::string& instruction,
    const std::string& endpointUrl,
    const std::string& modelName,
    const std::string& outputVmdPath,
    const std::string& captureDir,
    int maxIterations,
    int delaySeconds);
