#ifndef SIX_SEVEN_BOOT_LOGO_INSTALLER_H
#define SIX_SEVEN_BOOT_LOGO_INSTALLER_H

#include <string>
#include <vector>

namespace six_seven {

/* Замена логотипа загрузки ПК (UEFI) картинкой из assets/sprites/loading/. */
class BootLogoInstaller {
public:
    static bool Install(std::wstring* outMessage, bool silent = false);
    static bool Uninstall(std::wstring* outMessage, bool silent = false);
    static void EnsureInstalledOnStartup();
    static void LaunchInstallElevated();
    static void LaunchUninstallElevated();

private:
    static std::wstring FindHackBgrtDir();
    static std::wstring BootLogoToolPath();
    static bool ExportLoadingFrames(const std::wstring& hackDir, std::wstring* err);
    static bool WriteConfig(const std::wstring& hackDir, const std::vector<std::wstring>& bmpNames);
    static bool RunSetupBatch(const std::wstring& hackDir, const wchar_t* commands,
                              std::wstring* err);
};

} /* namespace six_seven */

#endif
