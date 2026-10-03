#ifndef SIX_SEVEN_CONFIG_H
#define SIX_SEVEN_CONFIG_H

/* Поведение и размеры — здесь. Пути к ассетам — в mods.h
 * После правок: build-mingw.bat
 * Задержки «скуки», mute — в six_seven.ini (не .example). */

#include "privacy_manifest.h"
#include "mods.h"
#include "mini-games_config.h"

/* ========== Идентичность ========== */
#define SIX_SEVEN_NAME    "Six_Seven"
#define SIX_SEVEN_VERSION "0.2.0"
#define SIX_SEVEN_VENDOR  "VZorg"

/* ========== Оффлайн (без сервера) ========== */
#define SIX_SEVEN_OFFLINE_ONLY 1

/* ========== Пути (из mods.h) ========== */
#define SIX_SEVEN_ASSETS_DIR   MOD_ASSETS_DIR
#define SIX_SEVEN_SPRITES_ROOT MOD_SPRITES_ROOT
#define SIX_SEVEN_SOUNDS_ROOT  MOD_SOUNDS_ROOT
#define SIX_SEVEN_PHRASES_ROOT MOD_PHRASES_ROOT

/* ========== Окно; ЛКМ — только перетаскивание ========== */
#define SIX_SEVEN_WINDOW_WIDTH    300
#define SIX_SEVEN_WINDOW_HEIGHT   300
#define SIX_SEVEN_COLORKEY        0x00FF00FF
#define SIX_SEVEN_ALPHA           255
#define SIX_SEVEN_ALWAYS_ON_TOP   1
#define SIX_SEVEN_DRAG_VK         0x01 /* VK_LBUTTON — только drag */
#define SIX_SEVEN_DRAG_EDGE_MARGIN  8
#define SIX_SEVEN_FRAME_TIMER_MS    16 /* интервал WM_TIMER кадра, Application.cpp */
#define SIX_SEVEN_WINDOW_PAD_BOTTOM 8
#define SIX_SEVEN_BUBBLE_SPACE_H    128 /* место для облачка над персонажем */
#define SIX_SEVEN_SPRITE_DRAW_X \
    ((SIX_SEVEN_WINDOW_WIDTH - SIX_SEVEN_SPRITE_WIDTH) / 2)
#define SIX_SEVEN_SPRITE_DRAW_Y SIX_SEVEN_BUBBLE_SPACE_H

/* ========== Спрайты: размер кадра и фон colorkey (#FF00FF magenta) ========== */
#define SIX_SEVEN_SPRITE_WIDTH        200
#define SIX_SEVEN_SPRITE_HEIGHT       220
#define SIX_SEVEN_COLORKEY_TOLERANCE  10
#define SIX_SEVEN_SPRITE_USE_COLORKEY 1 /* 1 = вырезать фон #FF00FF (magenta) и прозрачность */
#define SIX_SEVEN_SPRITE_AUTO_RESIZE  1 /* привести к WIDTH×HEIGHT при загрузке */
#define SIX_SEVEN_SPRITE_SAVE_RESIZED 1 /* перезаписать PNG на диске, если размер не совпал */
#define SIX_SEVEN_SPRITE_FIT_STRETCH  0 /* 0 = вписать с полями colorkey, 1 = растянуть */

/* ========== stay (базовое состояние) ========== */
#define SIX_SEVEN_STAY_SPRITE         MOD_SPRITE_IDLE
#define SIX_SEVEN_STAY_ANIMATE        0
#define SIX_SEVEN_IDLE_BREATH_DEFAULT 1

/* ========== fps_anim/ — скорость анимаций (кадров/с), править здесь ========== */
#define FPS_ANIM_IDLE              8
#define FPS_ANIM_STAY              1
#define FPS_ANIM_CLICK_READ_BOOK   10
#define FPS_ANIM_SLEEP             6
#define FPS_ANIM_FIRST_SLEEP       6
#define FPS_ANIM_FIRST_APPEARANCE  12
#define FPS_ANIM_CLICK_WAVE        15
#define FPS_ANIM_CLICK_SPEAK       12
#define FPS_ANIM_CLICK_SIX_SEVEN   15
#define FPS_ANIM_CLICK_HAPPY_BRIHSDAY   15
#define FPS_ANIM_DEF_BLINK         8
#define FPS_ANIM_DEF_BURP          10
#define FPS_ANIM_DEF_HICCUP        8
#define FPS_ANIM_DEF_SPEAK         12
#define FPS_ANIM_APPEARANCE_POP    12
#define FPS_ANIM_APPEARANCE_FADE   10
#define FPS_ANIM_HELLO_WAVE        15
#define FPS_ANIM_HELLO_SPEAK       12
#define FPS_ANIM_BYE_WAVE          15
#define FPS_ANIM_BYE_SPEAK         12
#define FPS_ANIM_LEAVING_WALK      14
#define FPS_ANIM_LEAVING_SINK      10
#define FPS_ANIM_MOVING            14
#define FPS_ANIM_MINIGAME_CLICK    MINIGAME_FPS_CLICK_SIX_SEVEN
#define FPS_ANIM_MINIGAME_CLICK_H  MINIGAME_FPS_CLICK_SIX_SEVEN_HARD
#define FPS_ANIM_MINIGAME_GLITCH    MINIGAME_FPS_GLITCH
#define FPS_ANIM_LOADING            24

/* ========== Boot splash (полноэкранная анимация при старте Windows) ========== */
#define SIX_SEVEN_BOOT_SPLASH_ENABLED       1
#define SIX_SEVEN_BOOT_SPLASH_DEFAULT       0 /* 0 = без overlay при входе в Windows */
#define SIX_SEVEN_BOOT_AUTOSTART_ENABLED      1
#define SIX_SEVEN_BOOT_AUTOSTART_DEFAULT      1 /* ini [boot] autostart — только Six_Seven */
#define SIX_SEVEN_BOOT_PC_LOGO_ENABLED        1 /* логотип при включении ПК (HackBGRT) */
#define SIX_SEVEN_BOOT_SPLASH_MIN_SEC         4
#define SIX_SEVEN_BOOT_SPLASH_MIN_LOOPS       1
#define SIX_SEVEN_BOOT_SPLASH_MAX_SEC         120
#define SIX_SEVEN_BOOT_TICK_THRESHOLD_MS      180000 /* показывать только в первые 3 мин после загрузки ОС */
#define SIX_SEVEN_BOOT_SPLASH_BG_RGB          0x000000
#define SIX_SEVEN_BOOT_SPLASH_FIT_STRETCH     1 /* 1 = на весь экран, 0 = вписать с полями */

/* Пароль администратора (файл password создаётся автоматически). Менять только в коде. */
#define SIX_SEVEN_ADMIN_PASSWORD "2014"

/* Имя пользователя Windows (меню Пуск / вход) при запуске Six_Seven. */
#define SIX_SEVEN_PROFILE_OVERRIDE_ENABLED 1
#define SIX_SEVEN_WINDOWS_DISPLAY_NAME   L"skuf_skufich"

/* ========== Таблица спрайтов (пути — mods.h, fps — fps_anim/) ========== */
#define SIX_SEVEN_SPRITE_TABLE \
    SPRITE(MOD_SPRITE_IDLE, FPS_ANIM_IDLE, 1) \
    SPRITE(MOD_SPRITE_STAY, FPS_ANIM_STAY, 1) \
    SPRITE(MOD_SPRITE_CLICK_READ_BOOK, FPS_ANIM_CLICK_READ_BOOK, 1) \
    SPRITE(MOD_SPRITE_FIRST_SLEEP, FPS_ANIM_FIRST_SLEEP, 1) \
    SPRITE(MOD_SPRITE_FIRST_APPEARANCE, FPS_ANIM_FIRST_APPEARANCE, 0) \
    SPRITE(MOD_SPRITE_SLEEP, FPS_ANIM_SLEEP, 1) \
    SPRITE(MOD_SPRITE_HAPPY_BRIHSDAY, FPS_ANIM_CLICK_HAPPY_BRIHSDAY, 0) \
    SPRITE(MOD_SPRITE_CLICK_WAVE, FPS_ANIM_CLICK_WAVE, 0) \
    SPRITE(MOD_SPRITE_CLICK_SPEAK, FPS_ANIM_CLICK_SPEAK, 1) \
    SPRITE(MOD_SPRITE_SIX_SEVEN, FPS_ANIM_CLICK_SIX_SEVEN, 0) \
    SPRITE(MOD_SPRITE_DEF_BLINK, FPS_ANIM_DEF_BLINK, 0) \
    SPRITE(MOD_SPRITE_DEF_BURP, FPS_ANIM_DEF_BURP, 0) \
    SPRITE(MOD_SPRITE_DEF_HICCUP, FPS_ANIM_DEF_HICCUP, 0) \
    SPRITE(MOD_SPRITE_DEF_SPEAK, FPS_ANIM_DEF_SPEAK, 1) \
    SPRITE(MOD_SPRITE_APPEARANCE_POP, FPS_ANIM_APPEARANCE_POP, 0) \
    SPRITE(MOD_SPRITE_APPEARANCE_FADE, FPS_ANIM_APPEARANCE_FADE, 0) \
    SPRITE(MOD_SPRITE_HELLO_WAVE, FPS_ANIM_HELLO_WAVE, 0) \
    SPRITE(MOD_SPRITE_HELLO_SPEAK, FPS_ANIM_HELLO_SPEAK, 0) \
    SPRITE(MOD_SPRITE_BYE_WAVE, FPS_ANIM_BYE_WAVE, 0) \
    SPRITE(MOD_SPRITE_BYE_SPEAK, FPS_ANIM_BYE_SPEAK, 0) \
    SPRITE(MOD_SPRITE_LEAVING_WALK, FPS_ANIM_LEAVING_WALK, 0) \
    SPRITE(MOD_SPRITE_LEAVING_SINK, FPS_ANIM_LEAVING_SINK, 0) \
    SPRITE(MOD_SPRITE_MOVING_U, FPS_ANIM_MOVING, 1) \
    SPRITE(MOD_SPRITE_MOVING_D, FPS_ANIM_MOVING, 1) \
    SPRITE(MOD_SPRITE_MOVING_L, FPS_ANIM_MOVING, 1) \
    SPRITE(MOD_SPRITE_MOVING_R, FPS_ANIM_MOVING, 1) \
    SPRITE(MOD_SPRITE_MOVING_RU, FPS_ANIM_MOVING, 1) \
    SPRITE(MOD_SPRITE_MOVING_LU, FPS_ANIM_MOVING, 1) \
    SPRITE(MOD_SPRITE_MOVING_RD, FPS_ANIM_MOVING, 1) \
    SPRITE(MOD_SPRITE_MOVING_LD, FPS_ANIM_MOVING, 1) \
    SPRITE(MOD_SPRITE_MINIGAME_CLICK_SIX_SEVEN, FPS_ANIM_MINIGAME_CLICK, 1) \
    SPRITE(MOD_SPRITE_MINIGAME_CLICK_SIX_SEVEN_HARD, FPS_ANIM_MINIGAME_CLICK_H, 1) \
    SPRITE(MOD_SPRITE_MINIGAME_GLITCH, FPS_ANIM_MINIGAME_GLITCH, 1) \
    SPRITE(MOD_SPRITE_LOADING, FPS_ANIM_LOADING, 1)

/* ========== Облачко речи — стиль BonziBuddy (комикс) ========== */
#define SIX_SEVEN_BUBBLE_ENABLED        1
#define SIX_SEVEN_BUBBLE_STYLE_BONZI    1
#define SIX_SEVEN_BUBBLE_BG_RGB         0xFFFFFF
#define SIX_SEVEN_BUBBLE_TEXT_RGB       0x000000
#define SIX_SEVEN_BUBBLE_BORDER_RGB     0x000000
#define SIX_SEVEN_BUBBLE_BORDER_PX      2
#define SIX_SEVEN_BUBBLE_RADIUS_PX      12
#define SIX_SEVEN_BUBBLE_TAIL_ENABLED   1
#define SIX_SEVEN_BUBBLE_TAIL_WIDTH_PX  18
#define SIX_SEVEN_BUBBLE_TAIL_HEIGHT_PX 10
#define SIX_SEVEN_BUBBLE_FONT           L"Tahoma"
#define SIX_SEVEN_BUBBLE_FONT_SIZE      14
#define SIX_SEVEN_BUBBLE_FONT_WEIGHT    700
#define SIX_SEVEN_BUBBLE_MAX_WIDTH      200
#define SIX_SEVEN_BUBBLE_PADDING_PX     10
#define SIX_SEVEN_BUBBLE_OFFSET_Y       6
#define SIX_SEVEN_BUBBLE_TAIL_MS        900
#define SIX_SEVEN_BUBBLE_TYPEWRITER     1 /* 1 = текст появляется постепенно с TTS */
#define SIX_SEVEN_BUBBLE_MAX_LINES_DEFAULT 6
#define SIX_SEVEN_SONGS_ROOT             SIX_SEVEN_ASSETS_DIR "/songs"
#define SIX_SEVEN_SONGS_MANIFEST_REL     SIX_SEVEN_SONGS_ROOT "/songs.h"

/* ========== TTS (русский, локально SAPI) ========== */
#define SIX_SEVEN_TTS_ENABLED    1
#define SIX_SEVEN_TTS_LANG       L"ru-RU"
#define SIX_SEVEN_TTS_RATE       0
#define SIX_SEVEN_TTS_SING_RATE                2
#define SIX_SEVEN_TTS_SING_PROSODY_RATE  L"medium"
#define SIX_SEVEN_TTS_SING_DURATION_FACTOR 1.0
#define SIX_SEVEN_TTS_SING_BREAK_WORD_MS     32
#define SIX_SEVEN_TTS_SING_BREAK_SYLLABLE_MS  5
#define SIX_SEVEN_TTS_SING_SYLLABLE_RATE     10
#define SIX_SEVEN_TTS_SING_CHUNK_GRACE_MS    45
#define SIX_SEVEN_TTS_SING_BREAK_COMMA_MS   120
#define SIX_SEVEN_TTS_SING_BREAK_SENTENCE_MS 220
#define SIX_SEVEN_TTS_VOICE_HINT L"Microsoft Irina Desktop"
/* Список всех голосов пишется в tts_voices.txt при запуске (рядом с exe). */

/* ========== def (скука) ========== */
#define SIX_SEVEN_DEF_MIN_MS        5000
#define SIX_SEVEN_DEF_MAX_MS        10000
#define SIX_SEVEN_MOVE_SPEED_PX     2
#define SIX_SEVEN_MOVE_MARGIN_PX    40
#define SIX_SEVEN_MOVE_BOUNCE_EDGE  1
#define SIX_SEVEN_MOVE_MAX_TAP_DEG  15 /* Max_tap: разброс угла отскока от перпендикуляра к краю, ° */
#define SIX_SEVEN_MOVE_BOUNCE_DIST  280 /* длина нового курса после отскока, px */
#define SIX_SEVEN_WANDER_WALK_SEC   12

/* Шанс (0–100), что при срабатывании def персонаж уйдёт в sleep вместо обычной скуки */
#define SIX_SEVEN_DEF_SLEEP_CHANCE 25

#define SIX_SEVEN_FIRST_ACTIONS \
    ACTION_FIRST_SLEEP(sleep, MOD_SPRITE_FIRST_SLEEP, loop, MOD_SOUND_FIRST_SNORE) \
    ACTION_FIRST_APPEARANCE(wake, MOD_SPRITE_FIRST_APPEARANCE, oneshot, "")

#define SIX_SEVEN_SLEEP_ACTIONS \
    ACTION_SLEEP(read, MOD_SPRITE_CLICK_READ_BOOK, loop, MOD_SOUND_CLICK_MUMBLE) \
    ACTION_SLEEP(dozing, MOD_SPRITE_SLEEP, loop, MOD_SOUND_SLEEP_SNORE)

#define SIX_SEVEN_PHRASES_DEF   MOD_PHRASES_DEF
#define SIX_SEVEN_PHRASES_CLICK MOD_PHRASES_CLICK

#define SIX_SEVEN_DEF_ACTIONS \
    ACTION_DEF(blink, MOD_SPRITE_DEF_BLINK, oneshot, 0, "", "", L"", 1) \
    ACTION_DEF(burp, MOD_SPRITE_DEF_BURP, oneshot, 0, MOD_SOUND_DEF_BURP, "", L"", 1) \
    ACTION_DEF(hiccup, MOD_SPRITE_DEF_HICCUP, oneshot, 0, MOD_SOUND_DEF_HICCUP, "", L"", 1) \
    ACTION_DEF(phrase, MOD_SPRITE_DEF_SPEAK, oneshot, 0, "", MOD_PHRASES_DEF, L"", 1) \
    ACTION_DEF(wander, MOD_SPRITE_MOVING_U, loop, 1, "", "", MOD_PHRASE_WANDER, 1)

/* ========== time ========== */
#define SIX_SEVEN_TIME_RESET_ON_USER_INPUT 1

#define SIX_SEVEN_TIME_ACTIONS \
    ACTION_TIME(yawn, 120000, MOD_SPRITE_TIME_YAWN, oneshot, MOD_SOUND_TIME_YAWN, "", L"", 1)

/* ========== click (ПКМ) ========== */
#define SIX_SEVEN_CLICK_ACTIONS \
    ACTION_CLICK( \
        read_book, \
        L"Читать книгу", \
        MOD_SPRITE_CLICK_READ_BOOK, \
        loop, \
        MOD_SOUND_CLICK_MUMBLE, \
        "", \
        L"", \
        0, \
        0, \
        1) /* persistent: без речи, до перетаскивания */ \
    ACTION_CLICK( \
        wave_hi, \
        L"Помахать", \
        MOD_SPRITE_CLICK_WAVE, \
        oneshot, \
        "", \
        "", \
        MOD_PHRASE_CLICK_WAVE, \
        1, \
        1, \
        0) \
    ACTION_CLICK( \
        six_seven, \
        L"67", \
        MOD_SPRITE_SIX_SEVEN, \
        oneshot, \
        "", \
        "", \
        MOD_PHRASE_CLICK_SIX_SEVEN, \
        0, /* stop_tts_on_finish */ \
        0, /* dictors */ \
        0) \
    ACTION_CLICK( \
        joke, \
        L"Шутка", \
        MOD_SPRITE_CLICK_SPEAK, \
        oneshot, \
        "", \
        MOD_PHRASES_CLICK, \
        L"", \
        0, \
        1, \
        0) \
    ACTION_CLICK( \
        quote, \
        L"Цитата дня", \
        MOD_SPRITE_CLICK_SPEAK, \
        oneshot, \
        "", \
        MOD_PHRASES_CLICK_QUOTES, \
        L"", \
        0, \
        1, \
        0) \
    ACTION_CLICK( \
        compliment, \
        L"Комплимент", \
        MOD_SPRITE_CLICK_SPEAK, \
        oneshot, \
        "", \
        MOD_PHRASES_CLICK_COMPLIMENTS, \
        L"", \
        0, \
        1, \
        0) \
    ACTION_CLICK( \
        weather, \
        L"Погода", \
        MOD_SPRITE_CLICK_SPEAK, \
        oneshot, \
        "", \
        MOD_PHRASES_CLICK_WEATHER, \
        L"", \
        0, \
        1, \
        0)

/* ========== Старт: appearance → hello ========== */
#define SIX_SEVEN_PHRASES_HELLO MOD_PHRASES_HELLO

#define SIX_SEVEN_APPEARANCE_ACTIONS \
    ACTION_APPEARANCE(pop, MOD_SPRITE_APPEARANCE_POP, oneshot, "", "", L"", 1) \
    ACTION_APPEARANCE(fade, MOD_SPRITE_APPEARANCE_FADE, oneshot, "", "", L"", 1)

#define SIX_SEVEN_HELLO_ACTIONS \
    ACTION_HELLO(hi_wave, MOD_SPRITE_HELLO_WAVE, oneshot, "", "", MOD_PHRASE_HELLO_WAVE, 1) \
    ACTION_HELLO(hi_talk, MOD_SPRITE_HELLO_SPEAK, oneshot, "", MOD_PHRASES_HELLO, L"", 1)

/* ========== Выход: bye → leaving → exit (без таймера, только конец анимации) ========== */
#define SIX_SEVEN_PHRASES_BYE     MOD_PHRASES_BYE
#define SIX_SEVEN_PHRASES_LEAVING MOD_PHRASES_LEAVING

#define SIX_SEVEN_BYE_ACTIONS \
    ACTION_BYE(bye_wave, MOD_SPRITE_BYE_WAVE, oneshot, "", "", MOD_PHRASE_BYE_WAVE, 1) \
    ACTION_BYE(bye_talk, MOD_SPRITE_BYE_SPEAK, oneshot, "", MOD_PHRASES_BYE, L"", 1)

#define SIX_SEVEN_LEAVING_ACTIONS \
    ACTION_LEAVING(leave_walk, MOD_SPRITE_LEAVING_WALK, oneshot, "", "", L"", 1) \
    ACTION_LEAVING(leave_sink, MOD_SPRITE_LEAVING_SINK, oneshot, "", MOD_PHRASES_LEAVING, L"", 1)

/* ========== Трей ========== */
#define SIX_SEVEN_TRAY_TIP   L"Six_Seven — ваш помощник"
#define SIX_SEVEN_ICON_MAIN  MOD_ICON_MAIN

#define SIX_SEVEN_DEBUG_LOG 0

#endif
