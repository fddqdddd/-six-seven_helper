#ifndef SIX_SEVEN_PRIVACY_MANIFEST_H
#define SIX_SEVEN_PRIVACY_MANIFEST_H

/*
 * Неизменяемые ограничения приватности Six_Seven.
 * Изменение значений на 1 требует явного ревью и обновления README.
 */

#ifndef SIX_SEVEN_ALLOW_NETWORK
#define SIX_SEVEN_ALLOW_NETWORK 0
#endif

#ifndef SIX_SEVEN_ALLOW_BROWSER_HOOKS
#define SIX_SEVEN_ALLOW_BROWSER_HOOKS 0
#endif

#ifndef SIX_SEVEN_ALLOW_TELEMETRY
#define SIX_SEVEN_ALLOW_TELEMETRY 0
#endif

#ifndef SIX_SEVEN_ALLOW_HOME_PAGE_CHANGE
#define SIX_SEVEN_ALLOW_HOME_PAGE_CHANGE 0
#endif

#ifndef SIX_SEVEN_ALLOW_FILE_SYSTEM_SCAN
#define SIX_SEVEN_ALLOW_FILE_SYSTEM_SCAN 0
#endif

#endif
