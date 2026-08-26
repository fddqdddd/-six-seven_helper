# Типы действий Six_Seven

Все настраивается в **`config.h`**. Спрайты — папки в `assets/sprites/<name>/`.

## Состояние `stay`

- Базовое состояние между действиями.
- Спрайт `stay` — **без** проигрывания кадров (`SIX_SEVEN_STAY_ANIMATE 0`) или один статичный кадр.
- Не путать с типом **`def`**.

## Тип `click`

- **ПКМ** по персонажу → меню.
- Каждая запись: `menu_label`, `sprite`, `sound`, `phrase` / `phrase_file`, `stop_tts_on_finish`.
- По окончании → **`stay`**.

## Тип `time`

- Планировщик срабатывает каждые **`delay_ms`** миллисекунд.
- Проигрывает привязанные спрайт, звук, фразу.
- Несколько записей = несколько независимых таймеров.

```cpp
ACTION_TIME(id, delay_ms, sprite, mode, sound, phrase_file, phrase_w)
```

## Тип `appearance`

- При **запуске** приложения (первая фаза).
- Несколько записей → **случайная** при каждом старте.

## Тип `hello`

- Сразу **после** `appearance` (вторая фаза старта).
- Несколько записей → случайная.
- Фразы, анимации, звуки настраиваются в таблице.

## Тип `bye`

- При **закрытии** (первая фаза выхода).
- Случайный выбор из таблицы.

## Тип `leaving`

- После `bye`, перед уничтожением окна.
- Случайный выбор (уход за край, исчезновение и т.д.).

### Порядок

| Событие | Цепочка |
|---------|---------|
| Старт | `appearance` → `hello` → `stay` |
| Закрытие | `bye` → `leaving` → exit |

## Тип `def`

- При **долгом игноре** (`SIX_SEVEN_DEF_MIN_MS` … `MAX_MS`).
- Примеры: моргание, рыгание, икота, фраза, **переезд** (`move=1`).
- Случайная запись из `SIX_SEVEN_DEF_ACTIONS`.
- После выполнения → **`stay`**.

## Компас `moving*`

Используется при `move=1` в `def` или явном перемещении:

| Спрайт | Направление |
|--------|-------------|
| `movingU` | вверх |
| `movingD` | вниз |
| `movingL` | влево |
| `movingR` | вправо |
| `movingRU` | вверх-вправо |
| `movingLU` | вверх-влево |
| `movingRD` | вниз-вправо |
| `movingLD` | вниз-влево |

`MovementEngine` вычисляет угол движения окна и подставляет имя спрайта.

## Макросы (план v0.2)

```cpp
#define SIX_SEVEN_MOVE_SPRITE_TABLE \
    SPRITE(movingU,  14, 1) \
    SPRITE(movingD,  14, 1) \
    /* ... остальные 6 ... */

#define SIX_SEVEN_DEF_ACTIONS \
    ACTION_DEF(blink, "def_blink", oneshot, 0, "", L"") \
    /* ... */

#define SIX_SEVEN_TIME_ACTIONS \
    ACTION_TIME(tick, 60000, "def_blink", oneshot, "", L"")

#define SIX_SEVEN_APPEARANCE_ACTIONS \
    ACTION_APPEARANCE(a1, "appearance_pop", oneshot, "", L"")

#define SIX_SEVEN_HELLO_ACTIONS \
    ACTION_HELLO(h1, "wave", oneshot, "", L"Привет!")

#define SIX_SEVEN_BYE_ACTIONS \
    ACTION_BYE(b1, "wave", oneshot, "", L"Пока!")

#define SIX_SEVEN_LEAVING_ACTIONS \
    ACTION_LEAVING(l1, "leaving_walk", oneshot, "", L"")
```

## Реализация (C++)

| Модуль | Роль |
|--------|------|
| `ActionRunner` | `Run(action)`, возврат в `stay` |
| `ActionScheduler` | таймеры `time` |
| `MovementEngine` | выбор `moving*` + `SetWindowPos` |
| `Lifecycle` | цепочки старта/выхода |
| `DefTimer` | скука → случайный `def` |
