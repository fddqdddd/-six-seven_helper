# Six_Seven

Виртуальный помощник для Windows (XP–11), C++, **MinGW**, полностью **оффлайн**.

Напоминает BonziBuddy по UX (облачко речи, озвучка, анимации), без слежки и сети.

## Решения проекта

| Тема | Выбор |
|------|--------|
| Арт | Рисует автор; в репо только структура папок |
| Сборка | **MinGW** (`build-mingw.bat`) |
| Сеть | **Нет** (`SIX_SEVEN_OFFLINE_ONLY`) |
| ЛКМ | Только **перетаскивание** |
| ПКМ | Меню действий `click` |
| Облачко | **Bonzi-style** — см. `docs/SPEECH_BUBBLE.md` |

## Assets

Папки по **типам**: `sprites/click/`, `sounds/def/`, `phrases/hello/` — см. `docs/ASSETS_LAYOUT.md`.

## Настройка

- **`mods.h`** — пути к анимациям, звукам, фразам.
- **`config.h`** — секция **`fps_anim/`**, размер окна, отскок (`MOVE_MAX_TAP_DEG`), облачко, TTS.
- **`six_seven.ini`** — задержки «скуки», mute, дыхание. Перезапуск exe.

## Сборка (MinGW)

1. Установить [MSYS2](https://www.msys2.org/) или MinGW-w64, добавить `bin` в PATH (`gcc`, `cmake`).
2. Запустить:

```bat
build-mingw.bat
```

Или вручную:

Рекомендуемый путь (латиница, без проблем MinGW):

`d:\VZorg\projects\67_helper`

Если папка ещё `проекты` — запустите **`rename-to-projects.bat`** (закройте Cursor), затем откройте проект из `projects\67_helper`.

```bat
cd /d d:\VZorg\projects\67_helper
build-mingw.bat
build-mingw\Six_Seven.exe
```

Положите PNG в `assets/sprites/...` (см. `docs/ASSETS_LAYOUT.md`). Без картинок — фиолетовые заглушки.

## Privacy

`privacy_manifest.h` — без сети, toolbar, телеметрии.

## Документация

- `docs/CUSTOM_ANIMATIONS.md` — **как сделать свои анимации** (кадры, config, действия)
- `docs/ACTION_TYPES.md` — типы click, time, def, appearance, hello, bye, leaving, stay, moving*
- `docs/SPEECH_BUBBLE.md` — облачко как у Bonzi
- `docs/SPRITE_FORMAT.md` — кратко: кадры PNG
