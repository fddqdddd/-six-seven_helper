#include "../include/SpeechEngine.h"
#include "../include/Util.h"
#include "../config.h"

#include <algorithm>
#include <cwctype>
#include <fstream>
#include <windows.h>
#include <sapi.h>

namespace six_seven {

namespace {

constexpr DWORD kSpeechStartGraceMs = 450;
constexpr DWORD kSongChunkGraceMs = SIX_SEVEN_TTS_SING_CHUNK_GRACE_MS;
constexpr LANGID kRussianLangId = 0x419;

std::wstring TokenString(ISpObjectToken* token, const wchar_t* key)
{
    if (!token)
        return {};
    LPWSTR value = nullptr;
    if (FAILED(token->GetStringValue(const_cast<wchar_t*>(key), &value)) || !value)
        return {};
    std::wstring out(value);
    CoTaskMemFree(value);
    return out;
}

std::wstring TokenId(ISpObjectToken* token)
{
    if (!token)
        return {};
    LPWSTR id = nullptr;
    if (FAILED(token->GetId(&id)) || !id)
        return {};
    std::wstring out(id);
    CoTaskMemFree(id);
    return out;
}

std::wstring TokenDisplayName(ISpObjectToken* token)
{
    std::wstring name = TokenString(token, L"Name");
    if (!name.empty())
        return name;
    const std::wstring id = TokenId(token);
    const size_t slash = id.find_last_of(L"\\/");
    if (slash != std::wstring::npos && slash + 1 < id.size())
        return id.substr(slash + 1);
    return id;
}

LANGID LangFromTokenId(const std::wstring& id)
{
    std::wstring upper = id;
    for (wchar_t& ch : upper)
        ch = static_cast<wchar_t>(towupper(ch));
    if (upper.find(L"RU-RU") != std::wstring::npos || upper.find(L"_RU_") != std::wstring::npos)
        return 0x419;
    if (upper.find(L"EN-US") != std::wstring::npos || upper.find(L"_EN-US_") != std::wstring::npos)
        return 0x409;
    if (upper.find(L"EN-GB") != std::wstring::npos)
        return 0x809;
    return 0;
}

LANGID TokenLanguage(ISpObjectToken* token)
{
    if (!token)
        return 0;
    LPWSTR langAttr = nullptr;
    if (SUCCEEDED(token->GetStringValue(L"Language", &langAttr)) && langAttr) {
        LANGID lid = static_cast<LANGID>(std::wcstoul(langAttr, nullptr, 16));
        CoTaskMemFree(langAttr);
        if (lid)
            return lid;
    }
    DWORD lang = 0;
    if (SUCCEEDED(token->GetDWORD(L"Language", &lang)) && lang)
        return static_cast<LANGID>(lang & 0xFFFF);
    return LangFromTokenId(TokenId(token));
}

std::wstring FriendlyVoiceLabel(ISpObjectToken* token)
{
    const std::wstring name = TokenString(token, L"Name");
    if (!name.empty())
        return name;
    std::wstring id = TokenDisplayName(token);
    for (wchar_t& ch : id)
        ch = static_cast<wchar_t>(towlower(ch));
    if (id.find(L"irina") != std::wstring::npos)
        return L"Microsoft Irina Desktop";
    if (id.find(L"pavel") != std::wstring::npos)
        return L"Microsoft Pavel Desktop";
    if (id.find(L"zira") != std::wstring::npos)
        return L"Microsoft Zira Desktop";
    if (id.find(L"david") != std::wstring::npos)
        return L"Microsoft David Desktop";
    if (id.find(L"mark") != std::wstring::npos)
        return L"Microsoft Mark";
    return TokenDisplayName(token);
}

bool NameMatchesHint(const std::wstring& name, const wchar_t* hint)
{
    if (!hint || !*hint)
        return true;
    std::wstring lower = name;
    std::wstring hintStr = hint;
    for (wchar_t& ch : lower)
        ch = static_cast<wchar_t>(towlower(ch));
    for (wchar_t& ch : hintStr)
        ch = static_cast<wchar_t>(towlower(ch));
    return lower.find(hintStr) != std::wstring::npos;
}

bool TokenMatchesHint(ISpObjectToken* token, const wchar_t* hint)
{
    if (!hint || !*hint)
        return true;
    return NameMatchesHint(FriendlyVoiceLabel(token), hint) ||
           NameMatchesHint(TokenId(token), hint);
}

ISpObjectToken* FindBestVoiceToken()
{
    ISpObjectTokenCategory* category = nullptr;
    if (FAILED(CoCreateInstance(CLSID_SpObjectTokenCategory, nullptr, CLSCTX_ALL,
                                IID_ISpObjectTokenCategory,
                                reinterpret_cast<void**>(&category))))
        return nullptr;

    if (FAILED(category->SetId(SPCAT_VOICES, FALSE))) {
        category->Release();
        return nullptr;
    }

    IEnumSpObjectTokens* enumerator = nullptr;
    if (FAILED(category->EnumTokens(nullptr, nullptr, &enumerator))) {
        category->Release();
        return nullptr;
    }
    category->Release();

    ISpObjectToken* fallbackLang = nullptr;
    ISpObjectToken* fallbackAny = nullptr;

    ULONG fetched = 0;
    ISpObjectToken* token = nullptr;
    while (enumerator->Next(1, &token, &fetched) == S_OK && fetched > 0) {
        const std::wstring name = FriendlyVoiceLabel(token);
        const LANGID lang = TokenLanguage(token);
        if (!fallbackAny) {
            fallbackAny = token;
            fallbackAny->AddRef();
        }
        if (lang == kRussianLangId && !fallbackLang) {
            fallbackLang = token;
            fallbackLang->AddRef();
        }
        if (lang == kRussianLangId && TokenMatchesHint(token, SIX_SEVEN_TTS_VOICE_HINT)) {
            enumerator->Release();
            if (fallbackAny)
                fallbackAny->Release();
            if (fallbackLang)
                fallbackLang->Release();
            return token;
        }
        token->Release();
        token = nullptr;
    }
    enumerator->Release();

    if (fallbackLang) {
        if (fallbackAny)
            fallbackAny->Release();
        return fallbackLang;
    }
    return fallbackAny;
}

void WriteInstalledVoicesList()
{
    ISpObjectTokenCategory* category = nullptr;
    if (FAILED(CoCreateInstance(CLSID_SpObjectTokenCategory, nullptr, CLSCTX_ALL,
                                IID_ISpObjectTokenCategory,
                                reinterpret_cast<void**>(&category))))
        return;
    if (FAILED(category->SetId(SPCAT_VOICES, FALSE))) {
        category->Release();
        return;
    }

    IEnumSpObjectTokens* enumerator = nullptr;
    if (FAILED(category->EnumTokens(nullptr, nullptr, &enumerator))) {
        category->Release();
        return;
    }
    category->Release();

    const std::wstring path = PathJoin(GetExeDirectory(), L"tts_voices.txt");
    std::ofstream out(WideToUtf8(path.c_str()), std::ios::binary | std::ios::trunc);
    if (!out)
        return;

    out << u8"# Six_Seven — установленные голоса SAPI 5\r\n";
    out << u8"# Имя | язык (LANGID) | пол | возраст\r\n";
    out << u8"# Выбор: config.h -> SIX_SEVEN_TTS_VOICE_HINT (подстрока имени)\r\n\r\n";

    ULONG fetched = 0;
    ISpObjectToken* token = nullptr;
    while (enumerator->Next(1, &token, &fetched) == S_OK && fetched > 0) {
        const std::wstring name = FriendlyVoiceLabel(token);
        const std::wstring gender = TokenString(token, L"Gender");
        const std::wstring age = TokenString(token, L"Age");
        wchar_t langBuf[16] = {};
        wsprintfW(langBuf, L"0x%04X", static_cast<unsigned>(TokenLanguage(token)));
        out << WideToUtf8(name.c_str()) << " | " << WideToUtf8(langBuf) << " | "
            << WideToUtf8(gender.c_str()) << " | " << WideToUtf8(age.c_str()) << " | "
            << WideToUtf8(TokenId(token).c_str()) << "\r\n";
        token->Release();
        token = nullptr;
    }
    enumerator->Release();
}

} /* namespace */

int SpeechEngine::EstimateDurationMs(const std::wstring& text) const
{
    if (text.empty())
        return 1;
    const double rateFactor = 1.0 - static_cast<double>(SIX_SEVEN_TTS_RATE) * 0.075;
    const int perChar = static_cast<int>(58 * std::max(0.35, rateFactor));
    int ms = std::max(400, static_cast<int>(text.size()) * perChar);
    if (singing_)
        ms = static_cast<int>(ms * static_cast<double>(SIX_SEVEN_TTS_SING_DURATION_FACTOR));
    return ms;
}

void SpeechEngine::UpdateVisibleProgress()
{
    if (songWordMode_ || fullText_.empty() || speakStartMs_ == 0)
        return;
    if (visibleChars_ >= fullText_.size())
        return;

    const DWORD elapsed = GetTickCount() - speakStartMs_;
    const int est = std::max(1, estimatedMs_);
    size_t next = (static_cast<size_t>(elapsed) * fullText_.size()) / static_cast<size_t>(est);
    if (next > fullText_.size())
        next = fullText_.size();
    if (next > visibleChars_)
        visibleChars_ = next;
}

void SpeechEngine::RestoreNormalRate()
{
#if SIX_SEVEN_TTS_ENABLED
    if (voice_)
        static_cast<ISpVoice*>(voice_)->SetRate(SIX_SEVEN_TTS_RATE);
#endif
}

void SpeechEngine::Poll()
{
#if SIX_SEVEN_TTS_ENABLED
    if (!voice_ || fullText_.empty())
        return;

    if (!songWordMode_)
        UpdateVisibleProgress();

    if (!speaking_)
        return;

    SPVOICESTATUS st = {};
    if (FAILED(static_cast<ISpVoice*>(voice_)->GetStatus(&st, nullptr)))
        return;

    if (st.dwRunningState == SPRS_IS_SPEAKING)
        return;

    if (GetTickCount() - speakStartMs_ <
        (songWordMode_ ? kSongChunkGraceMs : kSpeechStartGraceMs))
        return;

    speaking_ = false;

    if (songWordMode_) {
        if (songChunkIndex_ > 0 && songChunkIndex_ <= songChunks_.size())
            visibleChars_ = songChunks_[songChunkIndex_ - 1].reveal_through;
        if (visibleChars_ > fullText_.size())
            visibleChars_ = fullText_.size();
        SpeakNextSongChunk();
        return;
    }

    visibleChars_ = fullText_.size();
    if (pendingDone_) {
        auto cb = pendingDone_;
        pendingDone_ = nullptr;
        cb();
    }
#else
    (void)0;
#endif
}

bool SpeechEngine::Init()
{
#if SIX_SEVEN_TTS_ENABLED
    ISpVoice* voice = nullptr;
    if (FAILED(CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_ALL, IID_ISpVoice,
                                reinterpret_cast<void**>(&voice)))) {
        return false;
    }
    if (ISpObjectToken* token = FindBestVoiceToken()) {
        voice->SetVoice(token);
        token->Release();
    }
    voice->SetRate(SIX_SEVEN_TTS_RATE);
    voice_ = voice;
    WriteInstalledVoicesList();
    return true;
#else
    return false;
#endif
}

void SpeechEngine::Shutdown()
{
    Stop();
    if (voice_) {
        static_cast<ISpVoice*>(voice_)->Release();
        voice_ = nullptr;
    }
}

void SpeechEngine::Stop()
{
#if SIX_SEVEN_TTS_ENABLED
    if (voice_) {
        static_cast<ISpVoice*>(voice_)->Speak(nullptr, SPF_PURGEBEFORESPEAK, nullptr);
        RestoreNormalRate();
    }
#endif
    singing_ = false;
    songWordMode_ = false;
    speaking_ = false;
    pendingDone_ = nullptr;
    songOnDone_ = nullptr;
    songChunks_.clear();
    songChunkIndex_ = 0;
    fullText_.clear();
    visibleChars_ = 0;
    speakStartMs_ = 0;
}

void SpeechEngine::FinishSong()
{
    songWordMode_ = false;
    singing_ = false;
    speaking_ = false;
    songChunks_.clear();
    songChunkIndex_ = 0;
    visibleChars_ = fullText_.size();
    RestoreNormalRate();
    if (songOnDone_) {
        auto cb = std::move(songOnDone_);
        songOnDone_ = nullptr;
        cb();
    }
}

void SpeechEngine::SpeakNextSongChunk()
{
#if SIX_SEVEN_TTS_ENABLED
    if (!voice_) {
        FinishSong();
        return;
    }
    while (songChunkIndex_ < songChunks_.size() && songChunks_[songChunkIndex_].markup.empty())
        ++songChunkIndex_;

    if (songChunkIndex_ >= songChunks_.size()) {
        FinishSong();
        return;
    }

    const SongSpeakChunk& chunk = songChunks_[songChunkIndex_++];
    speakStartMs_ = GetTickCount();
    speaking_ = true;
    const HRESULT hr = static_cast<ISpVoice*>(voice_)->Speak(
        chunk.markup.c_str(), SPF_ASYNC | SPF_PURGEBEFORESPEAK | SPF_IS_XML, nullptr);
    if (FAILED(hr)) {
        visibleChars_ = fullText_.size();
        FinishSong();
    }
#else
    FinishSong();
#endif
}

void SpeechEngine::SpeakPayload(const std::wstring& payload, const std::wstring& displayText,
                                DWORD speakFlags, std::function<void()> onDone)
{
    pendingDone_ = std::move(onDone);
    fullText_ = displayText;
    visibleChars_ = 0;
    estimatedMs_ = EstimateDurationMs(displayText);
    speakStartMs_ = GetTickCount();

    if (payload.empty() || !voice_) {
        speaking_ = false;
        visibleChars_ = displayText.size();
        if (pendingDone_) {
            auto cb = pendingDone_;
            pendingDone_ = nullptr;
            cb();
        }
        return;
    }
#if SIX_SEVEN_TTS_ENABLED
    speaking_ = true;
    const HRESULT hr =
        static_cast<ISpVoice*>(voice_)->Speak(payload.c_str(), speakFlags, nullptr);
    if (FAILED(hr)) {
        speaking_ = false;
        visibleChars_ = displayText.size();
        RestoreNormalRate();
        singing_ = false;
        if (pendingDone_) {
            auto cb = pendingDone_;
            pendingDone_ = nullptr;
            cb();
        }
        return;
    }
#if !SIX_SEVEN_BUBBLE_TYPEWRITER
    visibleChars_ = displayText.size();
#endif
#else
    speaking_ = false;
    visibleChars_ = displayText.size();
    if (pendingDone_) {
        auto cb = pendingDone_;
        pendingDone_ = nullptr;
        cb();
    }
#endif
}

void SpeechEngine::Sing(const std::wstring& text, std::function<void()> onDone,
                        const SongMelodySpec& melody)
{
#if SIX_SEVEN_TTS_ENABLED
    if (voice_)
        static_cast<ISpVoice*>(voice_)->Speak(nullptr, SPF_PURGEBEFORESPEAK, nullptr);
    if (voice_)
        static_cast<ISpVoice*>(voice_)->SetRate(SIX_SEVEN_TTS_SING_RATE);
#endif
    singing_ = true;
    songWordMode_ = true;
    speaking_ = false;
    pendingDone_ = nullptr;
    fullText_ = text;
    visibleChars_ = 0;
    estimatedMs_ = 0;
    songOnDone_ = std::move(onDone);
    songChunks_ = BuildSongSpeakChunks(text, melody);
    songChunkIndex_ = 0;

    if (songChunks_.empty() || !voice_) {
        visibleChars_ = text.size();
        FinishSong();
        return;
    }
    SpeakNextSongChunk();
}

void SpeechEngine::Speak(const std::wstring& text, std::function<void()> onDone)
{
    songWordMode_ = false;
    songChunks_.clear();
    songChunkIndex_ = 0;
    songOnDone_ = nullptr;
    RestoreNormalRate();
    singing_ = false;
    SpeakPayload(text, text, SPF_ASYNC | SPF_PURGEBEFORESPEAK, std::move(onDone));
}

} /* namespace six_seven */
