# Раскладка assets

Арт добавляет автор — в репозитории только `.gitkeep` и примеры текстов.

## sprites/

Папки **по типам** (как в `config.h`):

```
sprites/
  stay/                 # статичная стойка (1 PNG)
  click/read_book/
  click/wave/
  click/speak/
  def/blink/
  def/burp/
  def/hiccup/
  def/speak/
  appearance/pop/
  appearance/fade/
  hello/wave/
  hello/speak/
  bye/wave/
  bye/speak/
  leaving/walk/
  leaving/sink/
  movingU/ movingD/ movingL/ movingR/
  movingRU/ movingLU/ movingRD/ movingLD/
```

Кадры: `0001.png`, `0002.png`, … Фон magenta `#FF00FF` для XP.

## sounds/

```
sounds/click/mumble.wav
sounds/def/burp.wav
sounds/def/hiccup.wav
sounds/time/yawn.wav
...
```

## phrases/

UTF-8, одна реплика на строку:

```
phrases/def/lines.txt
phrases/click/lines.txt
phrases/hello/lines.txt
phrases/bye/lines.txt
phrases/leaving/lines.txt
```
