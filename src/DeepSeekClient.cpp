#include "../include/DeepSeekClient.h"

#include "../include/Util.h"
#include "../config.h"

#include <windows.h>
#include <winhttp.h>

#include <cstdio>
#include <string>

namespace six_seven {

namespace {

/* Экранирование строки для JSON (url-часть тела). */
std::string JsonEscape(const std::wstring& text)
{
    std::string out;
    out.reserve(text.size() + 16);
    const std::string utf8 = WideToUtf8(text.c_str());
    for (char c : utf8) {
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                char buf[8];
                snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(c));
                out += buf;
            } else {
                out += c;
            }
            break;
        }
    }
    return out;
}

/* Декодирует JSON-escape из фрагмента "text" начиная с begin (после кавычки).
   Читает до закрывающей незаэкранированной кавычки, декодируя \n, \r, \\,
   \" и \uXXXX. Возвращает текст и позицию закрывающей кавычки. */
bool DecodeJsonString(const std::string& s, size_t begin, std::string& out, size_t& endPos)
{
    if (begin >= s.size() || s[begin] != '"')
        return false;
    out.clear();
    size_t i = begin + 1;
    for (; i < s.size(); ++i) {
        const char c = s[i];
        if (c == '"') {
            endPos = i;
            return true;
        }
        if (c == '\\') {
            if (i + 1 >= s.size())
                return false;
            const char e = s[i + 1];
            i += 1;
            switch (e) {
            case 'n':
                out += '\n';
                break;
            case 'r':
                out += '\r';
                break;
            case 't':
                out += '\t';
                break;
            case '\\':
                out += '\\';
                break;
            case '/':
                out += '/';
                break;
            case '"':
                out += '"';
                break;
            case 'u': {
                if (i + 4 >= s.size())
                    return false;
                unsigned code = 0;
                for (int k = 1; k <= 4; ++k) {
                    const char h = s[i + k];
                    code <<= 4;
                    if (h >= '0' && h <= '9')
                        code |= static_cast<unsigned>(h - '0');
                    else if (h >= 'a' && h <= 'f')
                        code |= static_cast<unsigned>(h - 'a' + 10);
                    else if (h >= 'A' && h <= 'F')
                        code |= static_cast<unsigned>(h - 'A' + 10);
                    else
                        return false;
                }
                i += 4;
                if (code < 0x80) {
                    out += static_cast<char>(code);
                } else if (code < 0x800) {
                    out += static_cast<char>(0xC0 | (code >> 6));
                    out += static_cast<char>(0x80 | (code & 0x3F));
                } else {
                    out += static_cast<char>(0xE0 | (code >> 12));
                    out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                    out += static_cast<char>(0x80 | (code & 0x3F));
                }
                break;
            }
            default:
                out += e;
                break;
            }
        } else {
            out += c;
        }
    }
    return false;
}

/* Извлекает первый "content":"..." из JSON-ответа. */
bool ExtractContent(const std::string& json, std::string& out)
{
    constexpr const char* marker = "\"content\"";
    const size_t pos = json.find(marker);
    if (pos == std::string::npos)
        return false;
    size_t i = pos + std::char_traits<char>::length(marker);
    while (i < json.size() && (json[i] == ' ' || json[i] == ':'))
        ++i;
    size_t endPos = 0;
    return DecodeJsonString(json, i, out, endPos);
}

} /* namespace */

std::wstring DeepSeekAsk(const std::wstring& apiKey, const std::wstring& userMessage,
                         std::wstring& errorMessage)
{
    errorMessage.clear();
    if (apiKey.empty()) {
        errorMessage = L"Пустой API-ключ";
        return {};
    }
    if (userMessage.empty()) {
        errorMessage = L"Пустое сообщение";
        return {};
    }

    HINTERNET session = WinHttpOpen(L"Six-Seven/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) {
        errorMessage = L"Не удалось открыть HTTP-сессию (код " +
                       std::to_wstring(GetLastError()) + L")";
        return {};
    }

    HINTERNET connect = WinHttpConnect(session, L"api.deepseek.com",
                                       INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!connect) {
        errorMessage = L"Не удалось подключиться к api.deepseek.com (код " +
                       std::to_wstring(GetLastError()) + L")";
        WinHttpCloseHandle(session);
        return {};
    }

    const std::wstring path = L"/chat/completions";
    HINTERNET request =
        WinHttpOpenRequest(connect, L"POST", path.c_str(), nullptr, WINHTTP_NO_REFERER,
                           WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!request) {
        errorMessage = L"Не удалось создать запрос (код " + std::to_wstring(GetLastError()) +
                       L")";
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return {};
    }

    DWORD timeouts = 30000;
    WinHttpSetTimeouts(session, timeouts, timeouts, timeouts, timeouts);

    std::string body =
        "{\"model\":\"deepseek-chat\",\"messages\":[{\"role\":\"user\",\"content\":\"" +
        JsonEscape(userMessage) + "\"}],\"max_tokens\":512,\"stream\":false}";

    std::wstring headers = L"Content-Type: application/json\r\nAuthorization: Bearer " +
                           apiKey + L"\r\n";

    if (!WinHttpSendRequest(request, headers.c_str(), static_cast<DWORD>(headers.size()), const_cast<char*>(body.data()),
                            static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0)) {
        errorMessage = L"Ошибка отправки запроса (код " + std::to_wstring(GetLastError()) + L")";
        goto cleanup;
    }
    if (!WinHttpReceiveResponse(request, nullptr)) {
        errorMessage = L"Нет ответа от сервера (код " + std::to_wstring(GetLastError()) + L")";
        goto cleanup;
    }

    {
        DWORD statusCode = 0;
        DWORD statusSize = sizeof(statusCode);
        if (WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize,
                                WINHTTP_NO_HEADER_INDEX)) {
            if (statusCode != 200) {
                wchar_t buf[64];
                swprintf(buf, 64, L"HTTP %lu", statusCode);
                errorMessage = buf;
                goto cleanup;
            }
        }

        std::string json;
        char buf[4096];
        DWORD read = 0;
        while (WinHttpReadData(request, buf, sizeof(buf), &read) && read > 0) {
            json.append(buf, read);
            read = 0;
        }

        std::string content;
        if (ExtractContent(json, content)) {
            WinHttpCloseHandle(request);
            WinHttpCloseHandle(connect);
            WinHttpCloseHandle(session);
            return Utf8ToWide(content.c_str());
        }
        errorMessage = L"Не удалось разобрать ответ";
    }

cleanup:
    if (request)
        WinHttpCloseHandle(request);
    if (connect)
        WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    return {};
}

} /* namespace six_seven */