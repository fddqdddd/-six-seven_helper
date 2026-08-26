#!/usr/bin/env python3
"""Generate notes.txt from lyrics (one note per Russian syllable)."""

import re
import sys
from pathlib import Path

VOWELS = set("аеёиоуыэюяАЕЁИОУЫЭЮЯ")


def is_vowel(ch: str) -> bool:
    return ch in VOWELS


def split_syllables(word: str) -> list[str]:
    if not word:
        return []
    vowel_pos = [i for i, ch in enumerate(word) if is_vowel(ch)]
    if not vowel_pos:
        return [word]
    syllables: list[str] = []
    start = 0
    for v, vowel_idx in enumerate(vowel_pos):
        end = len(word)
        if v + 1 < len(vowel_pos):
            between_start = vowel_idx + 1
            between_end = vowel_pos[v + 1]
            between_len = between_end - between_start
            if between_len == 0:
                end = vowel_idx + 1
            elif between_len == 1:
                end = vowel_idx + 1
            else:
                end = vowel_idx + 2
        if end > start:
            syllables.append(word[start:end])
        start = end
    if start < len(word):
        syllables.append(word[start:])
    return syllables or [word]


def tokenize(text: str) -> list[str]:
    words: list[str] = []
    current = ""
    punct = {",", ".", "!", "?", ";", "…", "—", "-"}
    for ch in text:
        if ch.isspace():
            if current:
                words.append(current)
                current = ""
            continue
        if ch in punct:
            if current:
                words.append(current)
                current = ""
            words.append(ch)
            continue
        current += ch
    if current:
        words.append(current)
    return words


def count_syllables(text: str) -> int:
    n = 0
    for w in tokenize(text):
        if len(w) == 1 and w in ",.!?;…—-":
            continue
        n += len(split_syllables(w))
    return n


NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]


def parse_note(name: str) -> int:
    i = 0
    letter = name[i].upper()
    i += 1
    semi = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}[letter]
    if i < len(name) and name[i] == "#":
        semi += 1
        i += 1
    elif i < len(name) and name[i].lower() == "b":
        semi -= 1
        i += 1
    octave = int(name[i]) if i < len(name) and name[i].isdigit() else 4
    if semi < 0:
        semi += 12
    return (octave + 1) * 12 + semi


def midi_to_name(midi: int) -> str:
    octave = midi // 12 - 1
    return f"{NOTE_NAMES[midi % 12]}{octave}"


def generate_notes(
    lyrics: str,
    ref: str,
    break_ms: int,
    step_sec: float,
    pattern: list[int],
) -> str:
    ref_midi = parse_note(ref)
    count = count_syllables(lyrics)
    lines = [
        f"ref {ref}",
        f"break {break_ms}",
        "# time note — auto-generated, one note per syllable",
    ]
    t = 0.0
    for i in range(count):
        semi = pattern[i % len(pattern)]
        midi = ref_midi + semi
        lines.append(f"{t:.2f} {midi_to_name(midi)}")
        t += step_sec
    return "\n".join(lines) + "\n"


ZHOPA_LYRICS = (
    "Бывает, человек с тобою груб, и холоден, как разовый прохожий. "
    "И резкие слова слетают с губ. Но жопой, жопой чувствуешь, хороший. "
    "Бывает, человек с тобою мил, кивает, говорит и смотрит мило. "
    "И непохож на форменных мудил. Но жопа, жопа чувствует, мудила. "
    "Я с ней бы разругалась в прах и пух, но скрыта в ней магическая сила, "
    "подводит всё, и зрение, и слух. А жопа никогда не подводила!"
)

ZHOPA_PATTERN = [
    0, 3, 5, 7, 7, 5, 3, 0, -2, 0, 3, 5, 7, 5, 3, 0,
    0, 3, 5, 7, 8, 7, 5, 3, 0, -2, 0, 3, 5, 7, 5, 3,
]

ZHOPium_LYRICS = (
    "Я мажу жопу вазелином, я так люблю анальный секс. "
    "Когда мужчина спит с мужчиной, мне симпатичен сей процесс. "
    "Кладу в карман презервативы, и выхожу на променад. "
    "Как много девушек красивых, а мне по-кайфу зад. "
    "Давай вечером с тобой трахнемся, ляжем вместе на кровать. "
    "Твоя задница мне так нравится, ну к чему это скрывать? "
    "Не прячь задницу, она жопиум для голубых, или для нас. "
    "Давай вечером с тобой трахнемся, один раз не пидорас. "
    "То ты меня, то я тебя, и нет плохого ничего. "
    "У нас с тобою два конца и это значит ого-го! "
    "И вот под музыку оркестра, а он для нас играет чушь, "
    "сегодня ты, а я невеста, а завтра ты мой муж. "
    "Давай вечером с тобой трахнемся, ляжем вместе на кровать. "
    "Твоя задница мне так нравится, ну к чему это скрывать? "
    "Не прячь задницу, она жопиум для голубых, или для нас. "
    "Давай вечером с тобой трахнемся, один раз не пидорас."
)

ZHOPium_PATTERN = [
    0, 3, 7, 7, 5, 3, 0, -2, 0, 3, 5, 7, 5, 3, 0, 0,
    3, 7, 10, 7, 5, 3, 0, -2, -3, 0, 3, 5, 7, 5, 3, 0,
    3, 7, 10, 12, 10, 7, 5, 3, 0, 3, 5, 7, 5, 3, 0, -2,
]


def main() -> None:
    root = Path(__file__).resolve().parents[1] / "assets" / "songs"
    songs = [
        ("zhopa_ne_podvodila", ZHOPA_LYRICS, "A3", 30, 0.26, ZHOPA_PATTERN),
        ("zhopium", ZHOPium_LYRICS, "A3", 30, 0.26, ZHOPium_PATTERN),
    ]
    for folder, lyrics, ref, brk, step, pattern in songs:
        d = root / folder
        d.mkdir(parents=True, exist_ok=True)
        (d / "lyrics.txt").write_text(lyrics, encoding="utf-8")
        notes = generate_notes(lyrics, ref, brk, step, pattern)
        (d / "notes.txt").write_text(notes, encoding="utf-8")
        n = count_syllables(lyrics)
        print(f"{folder}: {n} syllables, {n} notes")


if __name__ == "__main__":
    main()
