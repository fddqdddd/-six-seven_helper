# Облачко речи (стиль BonziBuddy)

При любой **озвучиваемой** фразе (TTS или текст из действия) над персонажем показывается облачко как у BonziBuddy / MS Agent:

## Внешний вид

- **Фон:** белый (`SIX_SEVEN_BUBBLE_BG_RGB`).
- **Рамка:** чёрная, 2 px (`SIX_SEVEN_BUBBLE_BORDER_PX`).
- **Углы:** скругление `RoundRect` (`SIX_SEVEN_BUBBLE_RADIUS_PX`).
- **Хвост:** треугольник внизу по центру, указывает на голову персонажа (`SIX_SEVEN_BUBBLE_TAIL_*`).
- **Текст:** чёрный, жирный Tahoma, перенос по `SIX_SEVEN_BUBBLE_MAX_WIDTH`.
- **Позиция:** над спрайтом, не перекрывает область drag тела персонажа.

## Поведение

1. Показать облачко с текстом фразы (UTF-8 → UTF-16).
2. Запустить SAPI TTS (ru-RU) параллельно.
3. По окончании TTS выдержать `SIX_SEVEN_BUBBLE_TAIL_MS`, затем скрыть.
4. При `stop_tts_on_finish` (например «Читать книгу») — скрыть облачко вместе с остановкой речи.

## Реализация (этап 3)

Модуль `SpeechBubble.cpp`: GDI на том же HDC, что и layered-окно (`OverlayWindow`).

Флаги в `config.h`: `SIX_SEVEN_BUBBLE_STYLE_BONZI`, `SIX_SEVEN_BUBBLE_ENABLED`.
