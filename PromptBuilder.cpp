#include "PromptBuilder.h"

std::string BuildMotionPrompt(const std::string& instruction, const std::vector<std::string>& boneNames)
{
    std::string prompt;
    prompt += "You are generating MMD motion JSON. Output JSON only.\n";
    prompt += "Instruction: " + instruction + "\n";
    prompt += "Bones: ";
    for (size_t i = 0; i < boneNames.size(); ++i) {
        prompt += boneNames[i];
        if (i + 1 < boneNames.size()) {
            prompt += ", ";
        }
    }
    prompt += "\n";
    prompt += "Schema:\n";
    prompt += "{\"header\":\"Vocaloid Motion Data 0002\",\"model_name\":\"Sour_Miku_Black\",\"bone_frames\":[{\"bone_name\":\"センター\",\"frame\":0,\"position\":[0,0,0],\"rotation\":[0,0,0,1]}]}\n";
    prompt += "Output only this schema with bone_frames.\n";
    prompt += "Use only the listed bones. Output only frames that change.\n";
    prompt += "rotation is a quaternion [x,y,z,w]. Use valid normalized quaternions.\n";
    prompt += "Use frames 0, 15, and 30 for any animated bone.\n";
    prompt += "Use rotation changes.\n";
    prompt += "Ensure at least one bone has different position or rotation between frames.\n";
    return prompt;
}
