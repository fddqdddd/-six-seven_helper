# Формат спрайтов Six_Seven

Спрайт и анимация — одно и то же: папка в `assets/sprites/<name>/`.

## Кадры

- Имена: `0001.png`, `0002.png`, … (4 цифры) или последовательные `.png`.
- Фон: magenta `#FF00FF` для colorkey на Windows XP (см. `SIX_SEVEN_COLORKEY`).
- Рекомендуемый размер кадра: до 200×220 px (вписывается в окно).

## Регистрация в config.h

```cpp
SPRITE(idle, 12, 1)   /* имя, fps, loop (1=да, 0=oneshot) */
```

Папка: `assets/sprites/idle/`.

## One-shot

Для `oneshot` движок вызывает `OnSpriteEnd` → `ActionRunner` возвращает в `idle` или останавливает TTS (`stop_tts_on_finish`).

Подробное руководство: **`docs/CUSTOM_ANIMATIONS.md`**.

## Placeholder

До появления арта можно положить один кадр `0001.png` — анимация будет статичной.
