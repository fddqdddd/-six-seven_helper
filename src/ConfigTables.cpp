#include "../config.h"
#include "../include/ActionTypes.h"
#include "../include/ConfigData.h"

#include <cstring>

namespace six_seven {

const SixSevenSpriteDef kSprites[] = { SIX_SEVEN_SPRITE_TABLE };
const int kSpriteCount = static_cast<int>(sizeof(kSprites) / sizeof(kSprites[0]));

const SixSevenActionDef kClickActions[] = { SIX_SEVEN_CLICK_ACTIONS };
const int kClickActionCount =
    static_cast<int>(sizeof(kClickActions) / sizeof(kClickActions[0]));

const SixSevenActionDef kTimeActions[] = { SIX_SEVEN_TIME_ACTIONS };
const int kTimeActionCount =
    static_cast<int>(sizeof(kTimeActions) / sizeof(kTimeActions[0]));

const SixSevenActionDef kDefActions[] = { SIX_SEVEN_DEF_ACTIONS };
const int kDefActionCount =
    static_cast<int>(sizeof(kDefActions) / sizeof(kDefActions[0]));

const SixSevenActionDef kSleepActions[] = { SIX_SEVEN_SLEEP_ACTIONS };
const int kSleepActionCount =
    static_cast<int>(sizeof(kSleepActions) / sizeof(kSleepActions[0]));

const SixSevenActionDef kAppearanceActions[] = { SIX_SEVEN_APPEARANCE_ACTIONS };
const int kAppearanceActionCount = static_cast<int>(
    sizeof(kAppearanceActions) / sizeof(kAppearanceActions[0]));

const SixSevenActionDef kHelloActions[] = { SIX_SEVEN_HELLO_ACTIONS };
const int kHelloActionCount =
    static_cast<int>(sizeof(kHelloActions) / sizeof(kHelloActions[0]));

const SixSevenActionDef kByeActions[] = { SIX_SEVEN_BYE_ACTIONS };
const int kByeActionCount =
    static_cast<int>(sizeof(kByeActions) / sizeof(kByeActions[0]));

const SixSevenActionDef kLeavingActions[] = { SIX_SEVEN_LEAVING_ACTIONS };
const int kLeavingActionCount =
    static_cast<int>(sizeof(kLeavingActions) / sizeof(kLeavingActions[0]));

const SixSevenActionDef kFirstActions[] = { SIX_SEVEN_FIRST_ACTIONS };
const int kFirstActionCount =
    static_cast<int>(sizeof(kFirstActions) / sizeof(kFirstActions[0]));

const SixSevenSpriteDef* FindSpriteDef(const char* path)
{
    for (int i = 0; i < kSpriteCount; ++i) {
        if (kSprites[i].path && path &&
            std::strcmp(kSprites[i].path, path) == 0)
            return &kSprites[i];
    }
    return nullptr;
}

} /* namespace six_seven */
