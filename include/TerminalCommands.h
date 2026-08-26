#ifndef SIX_SEVEN_TERMINAL_COMMANDS_H
#define SIX_SEVEN_TERMINAL_COMMANDS_H

#include <functional>
#include <string>

namespace six_seven {

bool InstallTerminalCommandStubs();
void PollTerminalCommandInbox(const std::function<void(const std::wstring&)>& onCommand);

} /* namespace six_seven */

#endif
