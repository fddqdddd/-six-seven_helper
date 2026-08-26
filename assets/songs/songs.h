# Six_Seven — каталог песен (assets/songs/songs.h)
#
# SONG id | заголовок | спрайт | loop | birthday | папка
#   loop 1/0 — анимация по кругу; birthday 1/0 — только в день рождения
#   папка — assets/songs/<папка>/lyrics.txt и notes.txt
#
# notes.txt:
#   ref C4       — опорная нота (0 полутонов для голоса)
#   break 45     — пауза между словами, мс
#   0.00 G4      — время (сек) и нота; одна нота на слог (RussianSyllables)

SONG birthday | С днём рождения тебя | click/happy-brihsday | 0 | 1 | birthday
SONG grupa_krovi | Группа крови — КИНО | click/speak | 1 | 0 | grupa_krovi
SONG eshche_ne_vecher | Ещё не вечер — В. Высоцкий | click/speak | 1 | 0 | eshche_ne_vecher
SONG chelovek_i_koshka | Человек и кошка — Ноль | click/speak | 1 | 0 | chelovek_i_koshka
SONG zhopa_ne_podvodila | А жопа никогда не подводила — Zlatentsia | click/speak | 1 | 0 | zhopa_ne_podvodila
SONG zhopium | Жопиум — пародия | click/speak | 1 | 0 | zhopium
