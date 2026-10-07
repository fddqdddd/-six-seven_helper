#ifndef SIX_SEVEN_MINI_GAMES_CONFIG_H
#define SIX_SEVEN_MINI_GAMES_CONFIG_H

/*
 * mini-games_config.h — настройки мини-игр (compile-time).
 * Рекорды хранятся в mini_games_records.ini рядом с exe.
 * Спрайты: assets/sprites/mini-games/
 */

/* ========== «Нажми меня» (click_six-seven) ========== */
#define MINIGAME_CLICK_ID                 "click_six_seven"
#define MINIGAME_CLICK_MENU_LABEL         L"Нажми меня"
#define MINIGAME_CLICK_RECORDS_LABEL      L"Нажми меня"

#define MINIGAME_CLICK_TIME_SEC           60  /* обычный режим, сек */
#define MINIGAME_CLICK_HARD_TIME_SEC      30  /* hard-mode, сек */
#define MINIGAME_CLICK_MOVE_INTERVAL_MS   900 /* пауза между прыжками */
#define MINIGAME_CLICK_HARD_MOVE_MS       320 /* hard-mode: быстрее */
#define MINIGAME_CLICK_SCORE_PER_HIT      1

#define MINIGAME_CLICK_PRESTART_TITLE     L"Нажми меня"
#define MINIGAME_CLICK_PRESTART_TEXT \
    L"Лови Six_Seven кликом по персонажу!\r\n" \
    L"Счёт — по центру экрана. Успей за отведённое время."

#define MINIGAME_CLICK_HARD_CHECK_LABEL   L"Hard-mode (глюки, 30 сек, быстрее)"

#define MINIGAME_CLICK_END_TITLE          L"Нажми меня — результат"
#define MINIGAME_CLICK_END_TEXT \
    L"Время вышло! Можно указать свой текст здесь в mini-games_config.h"

#define MINIGAME_CLICK_NEW_RECORD_SUFFIX  L"\r\n\r\nНовый рекорд!"

/* ========== FPS мини-игр (таблица спрайтов — config.h) ========== */
#define MINIGAME_FPS_CLICK_SIX_SEVEN      16
#define MINIGAME_FPS_CLICK_SIX_SEVEN_HARD 18
#define MINIGAME_FPS_GLITCH               10

/* ========== Hard-mode: глюки (только hard) ========== */
#define MINIGAME_GLITCH_GRAY_ENABLED      1
#define MINIGAME_GLITCH_GRAY_ALPHA        140   /* 0–255 */
#define MINIGAME_GLITCH_GRAY_INTERVAL_MS  6500
#define MINIGAME_GLITCH_GRAY_DURATION_MS  450

#define MINIGAME_GLITCH_SPRITE_ENABLED    1
#define MINIGAME_GLITCH_SPRITE_INTERVAL_MS 5000
#define MINIGAME_GLITCH_SPRITE_DURATION_MS 650

/* ========== Файл рекордов ========== */
#define MINIGAME_RECORDS_INI              "mini_games_records.ini"

/* ========== «На память» (shell game) ========== */
#define MINIGAME_MEMORY_ID                    "memory_shell"
#define MINIGAME_MEMORY_MENU_LABEL            L"На память"
#define MINIGAME_MEMORY_RECORDS_LABEL         L"На память"

#define MINIGAME_MEMORY_SLOT_COUNT            3   /* обычный: 1 настоящий + 2 фальш */
#define MINIGAME_MEMORY_HARD_SLOT_COUNT       4   /* hard: +1 фальш */

#define MINIGAME_MEMORY_PREVIEW_MS            1400
#define MINIGAME_MEMORY_DARKEN_MS             900
#define MINIGAME_MEMORY_DARK_BRIGHTNESS       0.20f
#define MINIGAME_MEMORY_REVEAL_MS             3000

#define MINIGAME_MEMORY_SHUFFLE_BASE_MS       3800
#define MINIGAME_MEMORY_SHUFFLE_ADD_MS        700  /* +каждый раунд */
#define MINIGAME_MEMORY_SWAP_START_MS         620
#define MINIGAME_MEMORY_SWAP_MIN_MS           160
#define MINIGAME_MEMORY_SWAP_STEP_MS          48
#define MINIGAME_MEMORY_SWAP_ANIM_MS          130

#define MINIGAME_MEMORY_HARD_SWAP_MULT        0.70f /* быстрее смена */
#define MINIGAME_MEMORY_HARD_SHUFFLE_MULT     1.30f /* дольше перемешивание */

#define MINIGAME_MEMORY_SLOT_DRAW_SIZE        200  /* px, кадр в слоте */
#define MINIGAME_MEMORY_SLOT_GAP              28
#define MINIGAME_MEMORY_PANEL_PAD             24

#define MINIGAME_MEMORY_SCARE_MS              2200
#define MINIGAME_MEMORY_SCARE_POP_MS          260  /* резкий выход на весь экран */
#define MINIGAME_MEMORY_SCARE_OVERSCAN        1.12f /* чуть больше экрана */

#define MINIGAME_MEMORY_PRESTART_TITLE        L"На память"
#define MINIGAME_MEMORY_PRESTART_TEXT \
    L"Запомни отличающегося Six_Seven!\r\n" \
    L"Сначала все темнеют, потом стаканчики меняются местами.\r\n" \
    L"Когда перемешивание закончится — нажми на правильного."

#define MINIGAME_MEMORY_HARD_CHECK_LABEL \
    L"Hard-mode (4 шляпки, глюки, быстрее и дольше)"

#define MINIGAME_MEMORY_END_WIN_TITLE         L"На память — победа!"
#define MINIGAME_MEMORY_END_WIN_TEXT \
    L"Раунд пройден! С каждым разом быстрее и дольше."

#define MINIGAME_MEMORY_END_LOSE_TITLE        L"На память — испуг!"
#define MINIGAME_MEMORY_END_LOSE_TEXT \
    L"Это был подделка! Игра окончена."

#define MINIGAME_MEMORY_NEW_RECORD_SUFFIX     L"\r\n\r\nНовый рекорд!"

/* ========== «Угадай число» (guess_number) ========== */
#define MINIGAME_GUESS_ID                 "guess_number"
#define MINIGAME_GUESS_MENU_LABEL         L"Угадай число"
#define MINIGAME_GUESS_RECORDS_LABEL      L"Угадай число"

#define MINIGAME_GUESS_NORMAL_RANGE       50   /* 1..50 */
#define MINIGAME_GUESS_NORMAL_ATTEMPTS    7
#define MINIGAME_GUESS_HARD_RANGE         100  /* 1..100 */
#define MINIGAME_GUESS_HARD_ATTEMPTS      9

#define MINIGAME_GUESS_TITLE              L"Угадай число"
#define MINIGAME_GUESS_HARD_CHECK_LABEL   L"Hard-mode (число до 100)"

/* ========== «Камень-ножницы-бумага» (rps) ========== */
#define MINIGAME_RPS_ID                   "rock_paper_scissors"
#define MINIGAME_RPS_MENU_LABEL           L"Камень-ножницы-бумага"
#define MINIGAME_RPS_RECORDS_LABEL        L"Камень-ножницы-бумага"

#define MINIGAME_RPS_ROUNDS               7
#define MINIGAME_RPS_TITLE                L"Камень-ножницы-бумага"
#define MINIGAME_RPS_HARD_CHECK_LABEL     L"Hard-mode (67 подглядывает по-крупному)"

/* ========== «Загадки» (riddles) ========== */
#define MINIGAME_RIDDLE_MENU_LABEL        L"Загадка от 67"
#define MINIGAME_RIDDLE_TITLE             L"Загадка от 67"

/* ========== «Змейка» (snake) ========== */
#define MINIGAME_SNAKE_ID                 "snake"
#define MINIGAME_SNAKE_MENU_LABEL         L"Змейка"
#define MINIGAME_SNAKE_RECORDS_LABEL      L"Змейка"

#define MINIGAME_SNAKE_COLS               20
#define MINIGAME_SNAKE_ROWS               14
#define MINIGAME_SNAKE_CELL               16 /* px */
#define MINIGAME_SNAKE_TICK_MS           150

/* ========== «Прятки» (hide_seek) ========== */
#define MINIGAME_HIDE_ID                  "hide_seek"
#define MINIGAME_HIDE_MENU_LABEL          L"Прятки"
#define MINIGAME_HIDE_RECORDS_LABEL       L"Прятки"

#define MINIGAME_HIDE_TIME_SEC            60   /* обычный режим, сек */
#define MINIGAME_HIDE_HARD_TIME_SEC       120  /* hard-mode, сек */

#define MINIGAME_HIDE_COUNTDOWN_DELAY_MS  1000 /* чёрный экран перед «3» */
#define MINIGAME_HIDE_COUNTDOWN_STEP_MS   1000 /* 3 → 2 → 1 */
#define MINIGAME_HIDE_FOUND_PAUSE_MS      3500 /* 67 показывается после находки */
#define MINIGAME_HIDE_POLL_MS             400  /* период проверки файла */
#define MINIGAME_HIDE_LOCK_HITS           2    /* подряд «файл открыт» до находки */
#define MINIGAME_HIDE_DELETE_ATTEMPTS     60   /* попыток удалить файл-приманку */
#define MINIGAME_HIDE_HARD_DEPTH_MIN      2    /* hard: уровней вложенности */
#define MINIGAME_HIDE_HARD_DEPTH_MAX      5

#define MINIGAME_HIDE_PRESTART_TITLE      L"Прятки"
#define MINIGAME_HIDE_PRESTART_TEXT \
    L"67 прячется: экран погаснет, пойдёт отсчёт 3-2-1,\r\n" \
    L"потом ищи файл-приманку в папках рабочего стола и открой его.\r\n" \
    L"Найдёшь — 67 покажется. Больше находок — лучше."

#define MINIGAME_HIDE_HARD_CHECK_LABEL \
    L"Hard-mode (файл до 5 уровней глубже, 120 сек)"

#define MINIGAME_HIDE_END_TITLE           L"Прятки — результат"
#define MINIGAME_HIDE_END_TEXT \
    L"Время вышло! 67 успела спрятаться хорошо."

#define MINIGAME_HIDE_FOUND_TEXT          L"Ты меня нашёл!"

#endif /* SIX_SEVEN_MINI_GAMES_CONFIG_H */
