#ifndef SIX_SEVEN_DEEPSEEK_CLIENT_H
#define SIX_SEVEN_DEEPSEEK_CLIENT_H

#include <string>

namespace six_seven {

/* Отправляет сообщение в DeepSeek chat completions API (api.deepseek.com)
   и возвращает текст ответа ассистента. При любой ошибке возвращает пустую
   строку и заполняет errorMessage. Вызов блокирующий (до ~общего таймаута). */
std::wstring DeepSeekAsk(const std::wstring& apiKey, const std::wstring& userMessage,
                         std::wstring& errorMessage);

} /* namespace six_seven */

#endif