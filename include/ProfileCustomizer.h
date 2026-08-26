#ifndef SIX_SEVEN_PROFILE_CUSTOMIZER_H
#define SIX_SEVEN_PROFILE_CUSTOMIZER_H

namespace six_seven {

/* Смена аватара и отображаемого имени Windows (Win10/11, HKLM + Public AccountPictures). */
class ProfileCustomizer {
public:
    /* Только после перезагрузки / включения ПК (свежая сессия Windows). */
    static void RequestApplyOnBootIfNeeded(bool onboarded);
    static void ScheduleApplyOnNextBoot();
    static void RequestInstallBootTask();
    static bool RequestFullSetupAndWait();
    static bool RequestApplyAndWait();
    static bool RequestRevertAndWait();
    static void RequestApply();
    static void RequestRevert();
    static bool ApplyBootElevated();
    static bool InstallBootApplyTask();
    static bool RemoveBootApplyTask();
    static bool ApplyElevated();
    static bool RevertElevated();
    static bool IsOverrideActive();
};

} /* namespace six_seven */

#endif
