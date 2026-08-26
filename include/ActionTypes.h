#ifndef SIX_SEVEN_ACTION_TYPES_H
#define SIX_SEVEN_ACTION_TYPES_H

/* Пути: assets/sprites/<sprite_path>/, sounds/<type>/, phrases/<type>/ */

#define SPRITE(path, fps, loop) \
    { path, fps, (loop) != 0 },

#define ACTION_CLICK( \
    id, menu_label, sprite_path, mode, sound, phrase_file, phrase_w, stop_tts, dictors, persistent) \
    { \
        SixSevenActionType::Click, \
        #id, \
        0, \
        menu_label, \
        sprite_path, \
        SixSevenSpriteMode::mode, \
        SixSevenOnFinish::ReturnStay, \
        phrase_file, \
        phrase_w, \
        sound, \
        0, \
        (stop_tts) != 0, \
        (dictors) != 0, \
        (persistent) != 0, \
    },

#define ACTION_TIME(id, delay_ms, sprite_path, mode, sound, phrase_file, phrase_w, dictors) \
    { \
        SixSevenActionType::Time, \
        #id, \
        delay_ms, \
        L"", \
        sprite_path, \
        SixSevenSpriteMode::mode, \
        SixSevenOnFinish::ReturnStay, \
        phrase_file, \
        phrase_w, \
        sound, \
        0, \
        0, \
        (dictors) != 0, \
        0, \
    },

#define ACTION_DEF(id, sprite_path, mode, move, sound, phrase_file, phrase_w, dictors) \
    { \
        SixSevenActionType::Def, \
        #id, \
        0, \
        L"", \
        sprite_path, \
        SixSevenSpriteMode::mode, \
        SixSevenOnFinish::ReturnStay, \
        phrase_file, \
        phrase_w, \
        sound, \
        (move) != 0, \
        0, \
        (dictors) != 0, \
        0, \
    },

#define ACTION_SLEEP(id, sprite_path, mode, sound) \
    { \
        SixSevenActionType::Sleep, \
        #id, \
        0, \
        L"", \
        sprite_path, \
        SixSevenSpriteMode::mode, \
        SixSevenOnFinish::ReturnStay, \
        "", \
        L"", \
        sound, \
        0, \
        0, \
        0, \
        1, \
    },

#define ACTION_APPEARANCE(id, sprite_path, mode, sound, phrase_file, phrase_w, dictors) \
    { \
        SixSevenActionType::Appearance, \
        #id, \
        0, \
        L"", \
        sprite_path, \
        SixSevenSpriteMode::mode, \
        SixSevenOnFinish::ChainNext, \
        phrase_file, \
        phrase_w, \
        sound, \
        0, \
        0, \
        (dictors) != 0, \
        0, \
    },

#define ACTION_HELLO(id, sprite_path, mode, sound, phrase_file, phrase_w, dictors) \
    { \
        SixSevenActionType::Hello, \
        #id, \
        0, \
        L"", \
        sprite_path, \
        SixSevenSpriteMode::mode, \
        SixSevenOnFinish::ReturnStay, \
        phrase_file, \
        phrase_w, \
        sound, \
        0, \
        0, \
        (dictors) != 0, \
        0, \
    },

#define ACTION_BYE(id, sprite_path, mode, sound, phrase_file, phrase_w, dictors) \
    { \
        SixSevenActionType::Bye, \
        #id, \
        0, \
        L"", \
        sprite_path, \
        SixSevenSpriteMode::mode, \
        SixSevenOnFinish::ChainNext, \
        phrase_file, \
        phrase_w, \
        sound, \
        0, \
        0, \
        (dictors) != 0, \
        0, \
    },

#define ACTION_LEAVING(id, sprite_path, mode, sound, phrase_file, phrase_w, dictors) \
    { \
        SixSevenActionType::Leaving, \
        #id, \
        0, \
        L"", \
        sprite_path, \
        SixSevenSpriteMode::mode, \
        SixSevenOnFinish::ExitApp, \
        phrase_file, \
        phrase_w, \
        sound, \
        0, \
        0, \
        (dictors) != 0, \
        0, \
    },

#define ACTION_FIRST_SLEEP(id, sprite_path, mode, sound) \
    { \
        SixSevenActionType::First, \
        #id, \
        0, \
        L"", \
        sprite_path, \
        SixSevenSpriteMode::mode, \
        SixSevenOnFinish::ReturnStay, \
        "", \
        L"", \
        sound, \
        0, \
        0, \
        0, \
        1, \
    },

#define ACTION_FIRST_APPEARANCE(id, sprite_path, mode, sound) \
    { \
        SixSevenActionType::First, \
        #id, \
        0, \
        L"", \
        sprite_path, \
        SixSevenSpriteMode::mode, \
        SixSevenOnFinish::ChainNext, \
        "", \
        L"", \
        sound, \
        0, \
        0, \
        0, \
        0, \
    },

enum class SixSevenActionType {
    Click,
    Time,
    Def,
    Sleep,
    Appearance,
    Hello,
    Bye,
    Leaving,
    First,
};

enum class SixSevenSpriteMode { loop, oneshot };

enum class SixSevenOnFinish {
    ReturnStay,
    ChainNext,
    ExitApp,
};

struct SixSevenSpriteDef {
    const char* path; /* относительно assets/sprites/, напр. "stay" или "click/read_book" */
    int fps;
    bool loop;
};

struct SixSevenActionDef {
    SixSevenActionType type;
    const char* id;
    unsigned delay_ms; /* только Time */
    const wchar_t* menu_label;
    const char* sprite_path;
    SixSevenSpriteMode sprite_mode;
    SixSevenOnFinish on_finish;
    const char* phrase_file;
    const wchar_t* phrase;
    const char* sound;
    bool move;
    bool stop_tts_on_finish;
    bool dictors; /* 1 = озвучка TTS для этого действия, 0 = только текст/облачко */
    bool persistent; /* 1 = до перетаскивания; таймер скуки на паузе */
};

#endif
