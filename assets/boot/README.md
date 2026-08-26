# Boot Assets

Папка для видео и звука при загрузке Windows.

## Файлы

- `video.mp4` — видео (H.264 MP4), показывается полноэкранно при старте
- `sound.wav` или `sound.mp3` — звук/песня, играет параллельно с видео

## Настройка

В `six_seven.ini`:

```ini
[videoboot]
enabled=1
```

## Запуск

```cmd
Six_Seven_VideoBoot.exe --boot
```

## Автозапуск

Для автозапуска при входе в Windows:
```cpp
VideoBootSplash::SyncAutostart(true);
```

Или добавьте вручную в `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`:
```
Six_Seven_VideoBoot = "C:\path\to\Six_Seven_VideoBoot.exe" --boot
```
