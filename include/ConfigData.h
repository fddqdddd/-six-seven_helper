#ifndef SIX_SEVEN_CONFIG_DATA_H
#define SIX_SEVEN_CONFIG_DATA_H

#include "ActionTypes.h"

namespace six_seven {

extern const SixSevenSpriteDef kSprites[];
extern const int kSpriteCount;

extern const SixSevenActionDef kClickActions[];
extern const int kClickActionCount;
extern const SixSevenActionDef kTimeActions[];
extern const int kTimeActionCount;
extern const SixSevenActionDef kDefActions[];
extern const int kDefActionCount;
extern const SixSevenActionDef kSleepActions[];
extern const int kSleepActionCount;
extern const SixSevenActionDef kAppearanceActions[];
extern const int kAppearanceActionCount;
extern const SixSevenActionDef kHelloActions[];
extern const int kHelloActionCount;
extern const SixSevenActionDef kByeActions[];
extern const int kByeActionCount;
extern const SixSevenActionDef kLeavingActions[];
extern const int kLeavingActionCount;
extern const SixSevenActionDef kFirstActions[];
extern const int kFirstActionCount;

const SixSevenSpriteDef* FindSpriteDef(const char* path);

} /* namespace six_seven */

#endif
