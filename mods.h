#ifndef SIX_SEVEN_MODS_H
#define SIX_SEVEN_MODS_H

/*
 * mods.h — пути к анимациям, звукам и фразам (без fps: fps → config.h, секция fps_anim/).
 * После правок: build-mingw.bat
 */

/* ========== Корни (относительно Six_Seven.exe) ========== */
#define MOD_ASSETS_DIR   "assets"
#define MOD_SPRITES_ROOT MOD_ASSETS_DIR "/sprites"
#define MOD_SOUNDS_ROOT  MOD_ASSETS_DIR "/sounds"
#define MOD_PHRASES_ROOT MOD_ASSETS_DIR "/phrases"
#define MOD_ICON_MAIN    MOD_ASSETS_DIR "/icons/six_seven.ico"

/* ========== подарки и маскировка (прятки/подарок) ========== */
#define MOD_HIDE_MASKIROVKA MOD_SPRITES_ROOT "/maskirovka" /* запасные файлы-приманки */
#define MOD_PRESENTS_DIR   MOD_ASSETS_DIR "/presents"   /* подарки на стол */
#define MOD_MASKING_DIR    MOD_ASSETS_DIR "/masking"    /* файлы-приманки для пряток */

/* ========== boot (логотип при включении ПК — assets/sprites/loading/) ========== */
#define MOD_SPRITE_LOADING "loading"

/* ========== аватар Windows (assets/sprites/avatar/) ========== */
#define MOD_SPRITE_AVATAR "avatar"

/* ========== Покой / idle ========== */
#define MOD_SPRITE_IDLE "idle"
#define MOD_SPRITE_STAY "stay"

/* ========== click (ПКМ) ========== */
#define MOD_SPRITE_CLICK_READ_BOOK "click/read_book"
#define MOD_SPRITE_CLICK_WAVE      "click/wave"
#define MOD_SPRITE_CLICK_SPEAK     "click/speak"
#define MOD_SPRITE_SIX_SEVEN       "click/six-seven"
#define MOD_SPRITE_HAPPY_BRIHSDAY  "click/happy-brihsday"
#define MOD_SOUND_CLICK_MUMBLE     MOD_SOUNDS_ROOT "/click/mumble.wav"

/* ===== click: действия на фразах (спрайт переиспользуется, файлы фраз в assets) ===== */
#define MOD_PHRASES_CLICK_QUOTES       MOD_PHRASES_ROOT "/click/quotes.txt"
#define MOD_PHRASES_CLICK_COMPLIMENTS  MOD_PHRASES_ROOT "/click/compliments.txt"
#define MOD_PHRASES_CLICK_WEATHER      MOD_PHRASES_ROOT "/click/weather.txt"

/* ========== first (первый запуск) ========== */
#define MOD_SPRITE_FIRST_SLEEP       "first/sleep"
#define MOD_SPRITE_FIRST_APPEARANCE  "first/appearance"
#define MOD_SOUND_FIRST_SNORE        MOD_SOUNDS_ROOT "/first/snore.wav"

/* ========== sleep (сон / чтение от скуки) ========== */
#define MOD_SPRITE_SLEEP           "sleep/dozing"
#define MOD_SOUND_SLEEP_SNORE      MOD_SOUNDS_ROOT "/sleep/snore.wav"
#define MOD_PHRASES_CLICK          MOD_PHRASES_ROOT "/click/lines.txt"
#define MOD_PHRASE_CLICK_READ      L"Буб-бу-бу... читаю..."
#define MOD_PHRASE_CLICK_WAVE      L"Привет!"
#define MOD_PHRASE_CLICK_SIX_SEVEN L"67"
#define MOD_PHRASE_CLICK_HAPPY_BRIHSDAY L"С днём рожденья тебя,с днём рожденья тебя. С днём рожденья поздравляю... С днём рожденья тебя."

/* ========== def (скука) ========== */
#define MOD_SPRITE_DEF_BLINK  "def/blink"
#define MOD_SPRITE_DEF_BURP   "def/burp"
#define MOD_SPRITE_DEF_HICCUP "def/hiccup"
#define MOD_SPRITE_DEF_SPEAK  "def/speak"
#define MOD_SOUND_DEF_BURP    MOD_SOUNDS_ROOT "/def/burp.wav"
#define MOD_SOUND_DEF_HICCUP  MOD_SOUNDS_ROOT "/def/hiccup.wav"
#define MOD_SOUND_DEF_ANGRY   MOD_SOUNDS_ROOT "/mini-games/memory/boo.mp3"
#define MOD_PHRASES_DEF       MOD_PHRASES_ROOT "/def/lines.txt"
#define MOD_PHRASES_DEF_ANGRY MOD_PHRASES_ROOT "/def/angry.txt"
#define MOD_PHRASES_DEF_TYPING MOD_PHRASES_ROOT "/def/typing.txt"
#define MOD_PHRASES_DEF_APPS   MOD_PHRASES_ROOT "/def/apps.txt"
#define MOD_PHRASES_DEF_RIDDLES MOD_PHRASES_ROOT "/def/riddles.txt"
#define MOD_PHRASE_WANDER     L"Пойду прогуляюсь"

/* ========== time ========== */
#define MOD_SPRITE_TIME_YAWN MOD_SPRITE_DEF_BURP
#define MOD_SOUND_TIME_YAWN  MOD_SOUNDS_ROOT "/time/yawn.wav"

/* ========== appearance / hello ========== */
#define MOD_SPRITE_APPEARANCE_POP  "appearance/pop"
#define MOD_SPRITE_APPEARANCE_FADE "appearance/fade"
#define MOD_SPRITE_HELLO_WAVE      "hello/wave"
#define MOD_SPRITE_HELLO_SPEAK     "hello/speak"
#define MOD_PHRASES_HELLO          MOD_PHRASES_ROOT "/hello/lines.txt"
#define MOD_PHRASE_HELLO_WAVE      L"Ещё раз здравствуй, friend!"

/* ===== hello: приветствия по времени суток и праздникам (файлы берутся по context) ===== */
#define MOD_PHRASES_HELLO_MORNING  MOD_PHRASES_ROOT "/hello/morning.txt"
#define MOD_PHRASES_HELLO_DAY      MOD_PHRASES_ROOT "/hello/day.txt"
#define MOD_PHRASES_HELLO_EVENING  MOD_PHRASES_ROOT "/hello/evening.txt"
#define MOD_PHRASES_HELLO_NIGHT    MOD_PHRASES_ROOT "/hello/night.txt"
#define MOD_PHRASES_HELLO_NEWYEAR  MOD_PHRASES_ROOT "/hello/newyear.txt"
#define MOD_PHRASES_HELLO_FEB23    MOD_PHRASES_ROOT "/hello/feb23.txt"
#define MOD_PHRASES_HELLO_MARCH8   MOD_PHRASES_ROOT "/hello/march8.txt"
#define MOD_PHRASES_HELLO_BIRTHDAY MOD_PHRASES_ROOT "/hello/birthday.txt"

/* ========== bye / leaving ========== */
#define MOD_SPRITE_BYE_WAVE   "bye/wave"
#define MOD_SPRITE_BYE_SPEAK  "bye/speak"
#define MOD_PHRASES_BYE       MOD_PHRASES_ROOT "/bye/lines.txt"
#define MOD_PHRASE_BYE_WAVE   L"Пока-пока!"

#define MOD_SPRITE_LEAVING_WALK "leaving/walk"
#define MOD_SPRITE_LEAVING_SINK "leaving/sink"
#define MOD_PHRASES_LEAVING     MOD_PHRASES_ROOT "/leaving/lines.txt"

/* ========== движение (прогулка) ========== */
#define MOD_SPRITE_MOVING_U  "movingU"
#define MOD_SPRITE_MOVING_D  "movingD"
#define MOD_SPRITE_MOVING_L  "movingL"
#define MOD_SPRITE_MOVING_R  "movingR"
#define MOD_SPRITE_MOVING_RU "movingRU"
#define MOD_SPRITE_MOVING_LU "movingLU"
#define MOD_SPRITE_MOVING_RD "movingRD"
#define MOD_SPRITE_MOVING_LD "movingLD"

/* ========== mini-games ========== */
#define MOD_SPRITE_MINIGAME_CLICK_SIX_SEVEN      "mini-games/click_six-seven"
#define MOD_SPRITE_MINIGAME_CLICK_SIX_SEVEN_HARD "mini-games/click_six-seven_hard"
#define MOD_SPRITE_MINIGAME_GLITCH               "mini-games/glitch"
#define MOD_SPRITE_MINIGAME_MEMORY_REAL          "mini-games/memory/real"
#define MOD_SPRITE_MINIGAME_MEMORY_FAKE1         "mini-games/memory/fake1"
#define MOD_SPRITE_MINIGAME_MEMORY_FAKE2         "mini-games/memory/fake2"
#define MOD_SPRITE_MINIGAME_MEMORY_FAKE3         "mini-games/memory/fake3"
#define MOD_SPRITE_MINIGAME_MEMORY_REVEAL        "mini-games/memory/reveal"
#define MOD_SPRITE_MINIGAME_MEMORY_SCARE         "mini-games/memory/scare"
#define MOD_SOUND_MINIGAME_MEMORY_OK             MOD_SOUNDS_ROOT "/mini-games/memory/ok.wav"
#define MOD_SOUND_MINIGAME_MEMORY_FAIL           MOD_SOUNDS_ROOT "/mini-games/memory/scare.wav"

#endif /* SIX_SEVEN_MODS_H */
