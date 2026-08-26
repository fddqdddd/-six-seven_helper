#ifndef SIX_SEVEN_NAME_VALIDATOR_H
#define SIX_SEVEN_NAME_VALIDATOR_H

#include <string>

namespace six_seven {

enum class NameValidationResult {
    Ok,
    SixSevenName,
    HasDigits,
    Profanity,
};

NameValidationResult ValidateUserName(const std::wstring& name);
const wchar_t* NameRejectionPhrase(NameValidationResult result);

} /* namespace six_seven */

#endif
