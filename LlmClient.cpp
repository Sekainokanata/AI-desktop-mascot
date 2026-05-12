#include "LlmClient.h"

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <windows.h>
#include <winhttp.h>
#include <stdio.h>

#pragma comment(lib, "winhttp.lib")

namespace
{
   std::wstring ToWide(const std::string& value)
    {
        if (value.empty()) {
            return std::wstring();
        }
      int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
        std::wstring result(size, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), size);
        return result;
    }

    std::string Utf8ToSystem(const std::string& value)
    {
        if (value.empty()) {
            return {};
        }
        int wideSize = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
        if (wideSize <= 0) {
            return value;
        }
        std::wstring wide(static_cast<size_t>(wideSize), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), wide.data(), wideSize);
        int ansiSize = WideCharToMultiByte(CP_ACP, 0, wide.data(), wideSize, nullptr, 0, nullptr, nullptr);
        if (ansiSize <= 0) {
            return value;
        }
        std::string ansi(static_cast<size_t>(ansiSize), '\0');
        WideCharToMultiByte(CP_ACP, 0, wide.data(), wideSize, ansi.data(), ansiSize, nullptr, nullptr);
        return ansi;
    }

    std::string ReadAllBytes(HINTERNET request)
    {
        std::string response;
        DWORD available = 0;
        while (WinHttpQueryDataAvailable(request, &available) && available > 0) {
            std::vector<char> buffer(available);
            DWORD read = 0;
            if (!WinHttpReadData(request, buffer.data(), available, &read) || read == 0) {
                break;
            }
            response.append(buffer.data(), buffer.data() + read);
        }
        return response;
    }

    void LogWinHttpError(const char* message)
    {
        DWORD error = GetLastError();
        printf("[LlmClient] %s (error=%lu)\n", message, static_cast<unsigned long>(error));
    }

    std::string EscapeJsonString(const std::string& value)
    {
        std::string escaped;
        escaped.reserve(value.size());
        for (char c : value) {
            switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buffer[7] = {};
                    sprintf_s(buffer, "\\u%04x", static_cast<unsigned char>(c));
                    escaped += buffer;
                } else {
                    escaped += c;
                }
                break;
            }
        }
        return escaped;
    }

    std::string Base64Encode(const std::vector<unsigned char>& data)
    {
        static const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string encoded;
        encoded.reserve(((data.size() + 2) / 3) * 4);
        size_t i = 0;
        while (i + 2 < data.size()) {
            unsigned int triple = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
            encoded.push_back(alphabet[(triple >> 18) & 0x3F]);
            encoded.push_back(alphabet[(triple >> 12) & 0x3F]);
            encoded.push_back(alphabet[(triple >> 6) & 0x3F]);
            encoded.push_back(alphabet[triple & 0x3F]);
            i += 3;
        }
        if (i < data.size()) {
            unsigned int triple = data[i] << 16;
            if (i + 1 < data.size()) {
                triple |= data[i + 1] << 8;
            }
            encoded.push_back(alphabet[(triple >> 18) & 0x3F]);
            encoded.push_back(alphabet[(triple >> 12) & 0x3F]);
            if (i + 1 < data.size()) {
                encoded.push_back(alphabet[(triple >> 6) & 0x3F]);
                encoded.push_back('=');
            } else {
                encoded.push_back('=');
                encoded.push_back('=');
            }
        }
        return encoded;
    }

    bool ReadFileBytes(const std::string& path, std::vector<unsigned char>& data)
    {
        FILE* file = nullptr;
        if (fopen_s(&file, path.c_str(), "rb") != 0 || !file) {
            return false;
        }
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        fseek(file, 0, SEEK_SET);
        if (size <= 0) {
            fclose(file);
            return false;
        }
        data.resize(static_cast<size_t>(size));
        size_t readSize = fread(data.data(), 1, data.size(), file);
        fclose(file);
        return readSize == data.size();
    }

    std::string GetImageMimeType(const std::string& path)
    {
        std::string::size_type dot = path.find_last_of('.');
        std::string ext = (dot == std::string::npos) ? std::string() : path.substr(dot + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (ext == "jpg" || ext == "jpeg") {
            return "image/jpeg";
        }
        if (ext == "webp") {
            return "image/webp";
        }
        if (ext == "gif") {
            return "image/gif";
        }
        if (ext == "bmp") {
            return "image/bmp";//うまくいかないかも？
        }
        return "image/png";
    }
}

std::string RequestMotionJson(const std::string& endpointUrl, const std::string& modelName, const std::string& prompt)
{
    URL_COMPONENTS components = {};
    components.dwStructSize = sizeof(components);
    components.dwSchemeLength = static_cast<DWORD>(-1);
    components.dwHostNameLength = static_cast<DWORD>(-1);
    components.dwUrlPathLength = static_cast<DWORD>(-1);
    components.dwExtraInfoLength = static_cast<DWORD>(-1);

    std::wstring endpointWide = ToWide(endpointUrl);
    if (!WinHttpCrackUrl(endpointWide.c_str(), static_cast<DWORD>(endpointWide.size()), 0, &components)) {
      LogWinHttpError("WinHttpCrackUrl failed");
        return {};
    }

    std::wstring host(components.lpszHostName, components.dwHostNameLength);
    std::wstring path(components.lpszUrlPath, components.dwUrlPathLength);
    if (components.dwExtraInfoLength > 0) {
        path.append(components.lpszExtraInfo, components.dwExtraInfoLength);
    }

    HINTERNET session = WinHttpOpen(L"AI Mascot", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) {
      LogWinHttpError("WinHttpOpen failed");
        return {};
    }
    HINTERNET connection = WinHttpConnect(session, host.c_str(), components.nPort, 0);
    if (!connection) {
        LogWinHttpError("WinHttpConnect failed");
        WinHttpCloseHandle(session);
        return {};
    }

    DWORD flags = (components.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request = WinHttpOpenRequest(connection, L"POST", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) {
     LogWinHttpError("WinHttpOpenRequest failed");
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return {};
    }

   std::string body = "{\"model\":\"" + EscapeJsonString(modelName) + "\",\"messages\":[{\"role\":\"user\",\"content\":\"" + EscapeJsonString(prompt) + "\"}],\"temperature\":0.2}";
    std::wstring headers = L"Content-Type: application/json\r\n";

    BOOL sent = WinHttpSendRequest(request, headers.c_str(), static_cast<DWORD>(headers.size()), reinterpret_cast<void*>(body.data()), static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0);
   if (!sent) {
        LogWinHttpError("WinHttpSendRequest failed");
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return {};
    }
    if (!WinHttpReceiveResponse(request, nullptr)) {
        LogWinHttpError("WinHttpReceiveResponse failed");
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return {};
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    if (WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX)) {
        printf("[LlmClient] HTTP status: %lu\n", static_cast<unsigned long>(statusCode));
    }

   std::string response = ReadAllBytes(request);
    printf("[LlmClient] Response size: %zu\n", response.size());
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return response;
}

std::string RequestMotionEvaluationJson(const std::string& endpointUrl, const std::string& modelName, const std::string& instruction, const std::string& imagePath, const std::string& lastGeneratedJson)
{
    /*std::vector<unsigned char> imageBytes;
    if (!ReadFileBytes(imagePath, imageBytes)) {
        printf("[LlmClient] Failed to read image: %s\n", imagePath.c_str());
        return {};
    }
    std::string imageData = Base64Encode(imageBytes);
    std::string imageUrl = "data:" + GetImageMimeType(imagePath) + ";base64," + imageData;//[ERROR] [google/gemma-4-e4b] 'url' field must be a base64 encoded image 

    //std::string prompt = "Instruction: " + instruction + "\nReturn JSON: {\"ok\":true/false,\"reason\":\"...\"} only.";

    // 引数に現在のモーションJSON（文字列）を追加
    std::string prompt = "指示: " + instruction + "\n"
        "実行したモーションデータ: " + currentMotionJson + "\n" // これを追加
        "画像を見て、指示通りの動きか判定してください。\n"
        "返却形式: {\"ok\":true/false, \"analysis\":\"...\", \"advice\":\"...\"} のJSONのみ。";



    std::string body = "{\"model\":\"" + EscapeJsonString(modelName) + "\",\"messages\":[{\"role\":\"user\",\"content\":[{\"type\":\"text\",\"text\":\"" + EscapeJsonString(prompt) + "\"},{\"type\":\"image_url\",\"image_url\":{\"url\":\"" + EscapeJsonString(imageUrl) + "\"}}]}],\"temperature\":0.2}";
    */
    std::vector<unsigned char> imageBytes;
    if (!ReadFileBytes(imagePath, imageBytes)) {
        printf("[LlmClient] Failed to read image: %s\n", imagePath.c_str());
        return {};
    }
    std::string imageData = Base64Encode(imageBytes);
    std::string imageUrl = "data:" + GetImageMimeType(imagePath) + ";base64," + imageData;

    // --- 修正：プロンプトに直前の生成データを含め、出力JSONスキーマを指定 ---
    std::string prompt = "指示: " + instruction + "\n"
        "直前に適用したモーションデータ: " + lastGeneratedJson + "\n"
        "画像を見て、指示通りの動きか判定してください。\n"
        "もし指示通りでない場合、上記のモーションデータでどのボーンを動かした結果、実際にはどのような動きになってしまっているかを分析してください。\n"
        "返却形式は必ず以下のJSON形式のみにしてください:\n"
        "{\"ok\":true/false, \"detected_movement\":\"(例: 頭のボーンを回転させたため、首を振る動作をした)\", \"advice\":\"(例: 腕のボーンの数値を変更してください)\"}";

    std::string body = "{\"model\":\"" + EscapeJsonString(modelName) + "\",\"messages\":[{\"role\":\"user\",\"content\":[{\"type\":\"text\",\"text\":\"" + EscapeJsonString(prompt) + "\"},{\"type\":\"image_url\",\"image_url\":{\"url\":\"" + EscapeJsonString(imageUrl) + "\"}}]}],\"temperature\":0.2}";
    URL_COMPONENTS components = {};
    components.dwStructSize = sizeof(components);
    components.dwSchemeLength = static_cast<DWORD>(-1);
    components.dwHostNameLength = static_cast<DWORD>(-1);
    components.dwUrlPathLength = static_cast<DWORD>(-1);
    components.dwExtraInfoLength = static_cast<DWORD>(-1);

    std::wstring endpointWide = ToWide(endpointUrl);
    if (!WinHttpCrackUrl(endpointWide.c_str(), static_cast<DWORD>(endpointWide.size()), 0, &components)) {
        LogWinHttpError("WinHttpCrackUrl failed");
        return {};
    }

    std::wstring host(components.lpszHostName, components.dwHostNameLength);
    std::wstring path(components.lpszUrlPath, components.dwUrlPathLength);
    if (components.dwExtraInfoLength > 0) {
        path.append(components.lpszExtraInfo, components.dwExtraInfoLength);
    }

    HINTERNET session = WinHttpOpen(L"AI Mascot", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) {
        LogWinHttpError("WinHttpOpen failed");
        return {};
    }
    HINTERNET connection = WinHttpConnect(session, host.c_str(), components.nPort, 0);
    if (!connection) {
        LogWinHttpError("WinHttpConnect failed");
        WinHttpCloseHandle(session);
        return {};
    }

    DWORD flags = (components.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET request = WinHttpOpenRequest(connection, L"POST", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!request) {
        LogWinHttpError("WinHttpOpenRequest failed");
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return {};
    }

    std::wstring headers = L"Content-Type: application/json\r\n";
    BOOL sent = WinHttpSendRequest(request, headers.c_str(), static_cast<DWORD>(headers.size()), reinterpret_cast<void*>(body.data()), static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0);
    if (!sent) {
        LogWinHttpError("WinHttpSendRequest failed");
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return {};
    }
    if (!WinHttpReceiveResponse(request, nullptr)) {
        LogWinHttpError("WinHttpReceiveResponse failed");
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return {};
    }

    std::string response = ReadAllBytes(request);
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return response;
}
