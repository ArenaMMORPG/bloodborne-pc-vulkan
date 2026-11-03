// SPDX-License-Identifier: GPL-2.0-or-later
#include "common/types.h"
#include "bbport_game_menu.h"

#include <new>
#include <sys/mman.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "bbport_guest_hooks.h"
#include "bbport_settings.h"
#include <SDL3/SDL.h>
#include <fmt/format.h>

namespace BbGameMenu {
namespace {

using u8 = std::uint8_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

// The game's menu code (eboot 1.09 vaddrs; found from the System menu builder 0x1bb3ad0).
constexpr u64 MsgLookup = 0xf6f340;      ///< (repository, table, category, id) -> UTF-16 text
constexpr u64 AddPageItem = 0x1b4e2a0;   ///< (menu, name+help texts, page factory, out)
constexpr u64 MakeText = 0x1ae8cc0;      ///< (out 0x40 bytes, category, id): a text entry
constexpr u64 FactoryToFunction = 0x1b6ed90; ///< ({vtable, fn}, out std::function) -> its __f_
constexpr u64 CreatePage = 0x1bb4c70;    ///< (out, arg, content std::function): an options page
constexpr u64 OpenPage = 0x1b20900;      ///< (a, b, layout name, rows fn, 0, 0): the page's content
constexpr u64 ListPush = 0x1b1bbd0;      ///< (value list, {u32 value, text}): up to 32 values
constexpr u64 ListRow = 0x1b78ab0;   ///< (page, texts, int* value, value list, int* default): pop-up list
constexpr u64 PairPush = 0x1b2c200;  ///< (two-value list, {u8 value, text})
constexpr u64 PairRow = 0x1b2a100;   ///< (page, texts, u8* value, two-value list, u8* default): left/right
constexpr u64 SliderRow = 0x1b2ac00; ///< (page, texts, u8* value 0..10, u8* default)
/// (page, texts, int* value, value list, int* default): the value's text in the row, left/right
/// step through the list (the game's own multi-value rows, 0x1b20f74).
constexpr u64 SelectRow = 0x1b29370;
/// A SelectRow row (0xcf0 bytes, vtable 0x5339e90): `int* value` at +0x3c8, its values (0x48 bytes
/// each: u32 value, text) from +0x3d8 (8-aligned), their count at +0xce0, the list widget showing
/// them at +0x100.
constexpr std::size_t RowValue = 0x3c8, RowValues = 0x3d8, RowCount = 0xce0, RowList = 0x100;
/// (row, input, ..., xmm0 frame time): a SelectRow's input, each of the open page's rows every
/// frame (the game's menu thread).
constexpr u64 RowInput = 0x1b26b00;
/// (row + RowList): the row's list widget draws its values' texts (looked up again).
constexpr u64 ListDraw = 0x1ad06e0;
/// (page): updates the bottom StatusBar help text for the page's currently selected row.
constexpr u64 UpdatePageHelpBar = 0x1b1ab20;
/// (page, game_settings): Bloodborne's native System -> Controls page content builder.
constexpr u64 ControlsContent = 0x1b211a0;
/// Menu input button check helpers (const u8* input -> bool).
constexpr u64 IsConfirmPressed = 0x1adfec0;
constexpr u64 IsLeftPressed = 0x1ae0890;
constexpr u64 IsRightPressed = 0x1ae0970;
/// (row): the row shows its value's text (looked up again).
constexpr u64 RowShowValue = 0x1b2b7c0;
/// (row + RowList, count): the list widget's number of values.
constexpr u64 ListSetCount = 0x1acf2c0;
/// (row + RowList, index): the list widget moves to a value; (row + RowList): shows where it is.
constexpr u64 ListSelect = 0x1acf6b0, ListShow = 0x1acfb20;
/// The row's copy of its default value (the page's "Default").
constexpr std::size_t RowDefault = 0x3d4;
constexpr u64 FactoryVtable = 0x533d120; ///< a System item's {vtable, page factory fn}
constexpr u64 ContentVtable = 0x5343880; ///< an options page's {vtable, content fn}
/// The option page layout (optionsetting.gfx) the port's pages use, by name: its title's text
/// id and its rows. ControllSetting stacked its six rows upwards, and a row's pop-up list showed
/// the rows below through it: the runtime gives the game a copy with them stacked downwards, as
/// the game's own pop-up rows have them (src/runtime_file.c, menu_layout_file).
struct Layout {
    u64 name;
    u32 title, subtitle; ///< StaticText ids in the layout (0: none)
    std::size_t rows;
};
constexpr Layout ControlsLayout{0x4934065, 113020, 0, 6}; // "ControllSetting"
constexpr u32 TextName = 0xc8, TextHelp = 0xc9; // SP_menu text / SP_one-line help
constexpr u32 ControlsItem = 110000;            // "Controls" in the System menu: Keybinds follow it
constexpr u32 ScreenSoundItem = 110001;         // "Screen/Sound" in the System menu: Display/Effects follow it

const u8 MsgLookupPrologue[] = {0x89, 0xf0, 0x48, 0x8b, 0x77, 0x08, 0x48, 0x8b, 0x34, 0xc6};
const u8 AddPageItemPrologue[] = {0x55, 0x48, 0x89, 0xe5, 0x41, 0x57};
const u8 RowInputPrologue[] = {0x55, 0x48, 0x89, 0xe5, 0x41, 0x57};

u64 image_base = 0;
template <typename F>
F Game(u64 va) {
    return reinterpret_cast<F>(image_base + va);
}

// ---- Texts: ids from IdBase up, looked up by the game like its own messages. ----
constexpr u32 IdBase = 0x7f0000;
struct Text {
    std::u16string english, russian;
};
std::vector<Text> texts;

std::u16string Utf16(const char* utf8) {
    std::u16string out;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(utf8); *p;) {
        u32 c = *p++;
        if (c >= 0xf0) {
            c = (c & 7) << 18 | (p[0] & 0x3f) << 12 | (p[1] & 0x3f) << 6 | (p[2] & 0x3f);
            p += 3;
        } else if (c >= 0xe0) {
            c = (c & 15) << 12 | (p[0] & 0x3f) << 6 | (p[1] & 0x3f);
            p += 2;
        } else if (c >= 0xc0) {
            c = (c & 31) << 6 | (p[0] & 0x3f);
            p += 1;
        }
        if (c >= 0x10000) {
            c -= 0x10000;
            out.push_back(char16_t(0xd800 + (c >> 10)));
            out.push_back(char16_t(0xdc00 + (c & 0x3ff)));
        } else {
            out.push_back(char16_t(c));
        }
    }
    return out;
}
u32 AddText(const char* english, const char* russian) {
    texts.push_back({Utf16(english), Utf16(russian)});
    return IdBase + u32(texts.size() - 1);
}
/// The game's own language decides (its "Screen/Sound" text in Cyrillic: Russian, else English);
/// the port's menu language until the game has looked that text up.
enum class GameLanguage { Unknown, English, Russian };
std::atomic<GameLanguage> game_language{GameLanguage::Unknown};

bool RussianTexts() {
    const GameLanguage game = game_language.load();
    return game == GameLanguage::Unknown
               ? BbSettings::Get().menu_language == BbSettings::MenuLanguage::Russian
               : game == GameLanguage::Russian;
}

// Texts that change while the game runs (the chosen aspect ratio's resolutions, "Auto (21:9)",
// and the 5 dynamic rows inside the unified Controls page):
// fixed buffers rewritten in place, so a text the game holds stays valid.
constexpr u32 DynamicBase = IdBase + 0x8000;
constexpr std::size_t DynamicLength = 128;
struct DynamicText {
    char16_t english[DynamicLength]{}, russian[DynamicLength]{};
};
constexpr u32 OutputSlots = 12;   ///< BbSettings::OutputList
constexpr int ControlsSlotCount = 5; ///< 5 dynamic rows below Row 0 ("Section") in Controls
constexpr int ControlsSectionCount = 4;
std::array<DynamicText, 1 + OutputSlots + 3 * ControlsSlotCount> dynamic_texts{};
constexpr u32 AutoText = DynamicBase;                                       ///< "Auto (21:9)"
constexpr u32 OutputTexts = DynamicBase + 1;                                ///< the resolutions (OutputSlots)
constexpr u32 ControlsSlotName = OutputTexts + OutputSlots;                 ///< 5 dynamic row labels in Controls
constexpr u32 ControlsSlotHelp = ControlsSlotName + ControlsSlotCount;      ///< 5 dynamic row helps in Controls
constexpr u32 ControlsSlotVal = ControlsSlotHelp + ControlsSlotCount;       ///< 5 dynamic row values in Controls

void Store16(char16_t (&out)[DynamicLength], const char* utf8) {
    const std::u16string text = Utf16(utf8);
    const std::size_t n = std::min(text.size(), DynamicLength - 1);
    for (std::size_t i = 0; i < n; ++i) {
        __atomic_store_n(&out[i], text[i], __ATOMIC_RELAXED);
    }
    __atomic_store_n(&out[n], char16_t(0), __ATOMIC_RELAXED);
}
void SetDynamic(u32 id, const char* english, const char* russian) {
    DynamicText& t = dynamic_texts[id - DynamicBase];
    Store16(t.english, english);
    Store16(t.russian, russian);
}

bool OurText(u32 id) {
    return (id >= IdBase && id < IdBase + texts.size()) ||
           (id >= DynamicBase && id < DynamicBase + dynamic_texts.size());
}
const char16_t* TextOf(u32 id) {
    const bool russian = RussianTexts();
    if (id >= DynamicBase) {
        const DynamicText& t = dynamic_texts[id - DynamicBase];
        return russian ? t.russian : t.english;
    }
    const Text& t = texts[id - IdBase];
    return russian ? t.russian.c_str() : t.english.c_str();
}

// ---- The port's values the game's rows edit (ints: the list rows bind ints). ----
enum Field : int {
    Aspect, OutputRes, Upscaler, Preset, Sharpness, ModelLod,
    FirstEffect,
    ControlsSection = FirstEffect + BbSettings::EffectCount,
    FirstSlot,
    FieldCount = int(FirstSlot) + ControlsSlotCount
};
int values[FieldCount];  // what the game's rows show and change (list rows bind ints)
u8 bytes[FieldCount];    // the same for toggle and slider rows (they bind bytes)
bool byte_field[FieldCount]; // fields shown by toggle or slider rows
int default_values[FieldCount]; // the port's defaults: what the page's "Default" sets
u8 default_bytes[FieldCount];
int applied[FieldCount]; // what Poll applied last
std::atomic<bool> values_valid{false};
constexpr float SharpnessStep = 0.2f; // the game's slider: 0..10 -> 0.0 .. 2.0
constexpr int SharpnessSteps = 11;
constexpr float MouseSensStep = 0.2f; // 0 off, 1..10 -> 0.2 .. 2.0
constexpr int MouseSensSteps = 11;
constexpr int LodValues[] = {-2, 0, 1, 2};
/// The resolution row's values: the outputs of the aspect ratio it shows (value i: entry i). Set
/// when the page is built and when another aspect ratio is chosen on it (RefreshOutputRow, the
/// game's menu thread; Poll reads it once the rows have rested).
BbSettings::OutputList menu_outputs;
int output_row_aspect = -1; ///< the aspect ratio the resolution row shows

BbSettings::OutputSize MenuOutput(int aspect, int index) {
    if (menu_outputs.count == 0) return BbSettings::NearestOutput(aspect, BbSettings::OutputDefault);
    return BbSettings::NearestOutput(aspect, menu_outputs.sizes[std::clamp(index, 0, menu_outputs.count - 1)]);
}

/// The resolution row's value of `output` (0 if none).
int MenuOutputIndex(int aspect, BbSettings::OutputSize output) {
    for (int i = 0; i < menu_outputs.count; ++i) {
        if (MenuOutput(aspect, i) == output) return i;
    }
    return 0;
}

/// The resolution row's texts for `aspect`, "Auto (..)" for the monitor's aspect ratio.
void RefreshOutputs(int aspect) {
    const std::string group = BbSettings::AspectKey(BbSettings::ResolvedAspect(BbSettings::AspectAuto));
    SetDynamic(AutoText, ("Auto (" + group + ")").c_str(), ("Авто (" + group + ")").c_str());
    for (int i = 0; i < int(OutputSlots); ++i) {
        const auto o = MenuOutput(aspect, i);
        const std::string text = std::to_string(o.width) + " x " + std::to_string(o.height);
        SetDynamic(OutputTexts + u32(i), text.c_str(), text.c_str());
    }
}

/// The rows' values of settings `s`.
void Fill(const BbSettings::Values& s, int* values) {
    values[Aspect] = s.aspect;
    values[OutputRes] = MenuOutputIndex(s.aspect, BbSettings::NearestOutput(
                                                      s.aspect, BbSettings::UnpackSize(s.output_res)));
    values[Upscaler] = s.upscaler;
    values[Preset] = s.preset;
    // One slider for both: 0 is sharpening off.
    values[Sharpness] =
        s.sharpen ? std::clamp(int(std::lround(s.sharpness / SharpnessStep)), 1, SharpnessSteps - 1) : 0;
    values[ModelLod] = 1;
    for (int i = 0; i < 4; ++i) {
        if (LodValues[i] == s.model_lod) values[ModelLod] = i;
    }
    for (int e = 0; e < BbSettings::EffectCount; ++e) {
        values[FirstEffect + e] = s.effects[e];
    }
    values[ControlsSection] = std::clamp(values[ControlsSection], 0, ControlsSectionCount - 1);
    for (int slot = 0; slot < ControlsSlotCount; ++slot) {
        values[FirstSlot + slot] = 0;
    }
}

void LoadValues() {
    const int aspect = BbSettings::Get().aspect;
    menu_outputs = BbSettings::Outputs(aspect);
    RefreshOutputs(aspect);
    Fill(BbSettings::Get(), values);
    output_row_aspect = values[Aspect];
    for (int f = 0; f < FieldCount; ++f) {
        bytes[f] = u8(values[f]);
    }
    // "Default" on the page: the game's resolution in this list.
    default_values[OutputRes] =
        MenuOutputIndex(aspect, BbSettings::NearestOutput(aspect, BbSettings::OutputDefault));
    std::memcpy(applied, values, sizeof(values));
    values_valid = true;
}

// ---- Pages and rows ----
enum class Kind { List, Toggle, Slider, Select };
struct Row {
    Field field;
    u32 name, help;
    std::vector<u32> choices; ///< value texts, value = index (a toggle: off, on)
    Kind kind = Kind::List;
};
struct Page {
    u32 name, help;
    const Layout* layout;
    std::vector<Row> rows; ///< at most the layout's
};
std::vector<Page> pages;

std::vector<Row> controls_rows;

void DefineTexts() {
    const u32 off = AddText("Off", "Выкл"), on = AddText("On", "Вкл");
    const std::vector<u32> toggle = {off, on};
    const auto page = [](const char* en, const char* ru, const char* help_en, const char* help_ru,
                         const Layout& layout) {
        const u32 name = AddText(en, ru);
        pages.push_back({name, AddText(help_en, help_ru), &layout, {}});
    };
    const auto row = [](Field f, const char* en, const char* ru, const char* help_en,
                        const char* help_ru, std::vector<u32> choices, Kind kind = Kind::List) {
        if (kind == Kind::List && choices.size() == 2) {
            kind = Kind::Toggle; // left/right in place, as the game's own on/off rows
        }
        pages.back().rows.push_back(
            {f, AddText(en, ru), AddText(help_en, help_ru), std::move(choices), kind});
    };

    // Two pages of the ControllSetting layout's six rows (the game's other layouts are fixed
    // forms): the picture, then the game's effects. Choices are the game's pop-up rows
    // (SelectRow). The other patches (intros, the free camera, the debug menu), the game's own AA
    // and the FPS counter are in the overlay menu and the launcher.
    page("Display", "Изображение", "Aspect ratio, resolution, upscaler and detail (bbport)",
         "Соотношение сторон, разрешение, апскейлер и детализация (bbport)", ControlsLayout);
    row(Aspect, "Aspect ratio", "Соотношение сторон",
        "Of the final frame: wider sees more to the sides, the UI keeps to the screen edges",
        "Готового кадра: шире - больше видно по бокам, интерфейс у краёв экрана",
        [] {
            std::vector<u32> aspects = {AutoText};
            for (int a = BbSettings::AspectAuto + 1; a < BbSettings::AspectCount; ++a) {
                aspects.push_back(AddText(BbSettings::AspectKey(a), BbSettings::AspectKey(a)));
            }
            return aspects;
        }(),
        Kind::Select);
    {
        std::vector<u32> outputs;
        for (u32 i = 0; i < OutputSlots; ++i) outputs.push_back(OutputTexts + i);
        row(OutputRes, "Resolution", "Разрешение", "Size of the final frame and the UI",
            "Размер готового кадра и интерфейса", std::move(outputs), Kind::Select);
    }
    row(Upscaler, "Upscaler", "Апскейлер", "Temporal upscaler and anti-aliasing",
        "Временной апскейлер и сглаживание",
        {off, AddText("FSR 3.1", "FSR 3.1"), AddText("FSR 4", "FSR 4"), AddText("FSR 4.1.1", "FSR 4.1.1"),
         AddText("TAA", "TAA"), AddText("DLSS", "DLSS")},
        Kind::Select);
    row(Preset, "Quality preset", "Пресет", "Scene resolution relative to the output",
        "Разрешение сцены относительно вывода",
        {AddText("Native AA", "Native AA"), AddText("Quality", "Quality"), AddText("Balanced", "Balanced"),
         AddText("Performance", "Performance"), AddText("Ultra Performance", "Ultra Performance")},
        Kind::Select);
    row(Sharpness, "Sharpness", "Резкость", "RCAS after the upscaler: 0 off, 10 strongest",
        "RCAS после апскейлера: 0 выкл, 10 сильнее всего", {}, Kind::Slider);
    row(ModelLod, "Model detail", "Детализация", "Level of detail of models (after a restart)",
        "Детализация моделей (после перезапуска)",
        {AddText("Highest", "Максимальная"), AddText("Game default", "Как в игре"),
         AddText("Lower", "Ниже"), AddText("Lowest", "Минимальная")},
        Kind::Select);

    page("Effects", "Эффекты", "The game's effects (bbport; the first seven at once)",
         "Эффекты игры (bbport; первые семь сразу)", ControlsLayout);

    // Short names for the game's narrow name column (BbSettings::Effects order); the help line
    // has the details.
    struct Short {
        const char *key, *en, *ru, *help_en, *help_ru;
        bool graphics = true;
    };
    static const Short shorts[] = {
        {"effect_chromatic_aberration", "Chromatic aberration", "Хром. аберрация",
         "Colour fringes at the frame edges", "Цветные каймы по краям кадра"},
        {"effect_dof", "Depth of field", "Глубина резкости", "Blur of distant and near objects (DoF)",
         "Размытие дальних и близких объектов (DoF)"},
        {"effect_motion_blur", "Motion blur", "Размытие в движении", "Blur when the camera turns",
         "Размытие при повороте камеры"},
        {"effect_ssao", "Ambient occlusion", "Затенение SSAO", "Shading in corners and contacts (SSAO)",
         "Затенение в углах и местах касания (SSAO)"},
        {"effect_game_aa", "Game's own AA", "Сглаживание игры", "The game's own anti-aliasing",
         "Собственное сглаживание игры", false},
        {"effect_dynamic_shadows", "Dynamic shadows", "Динамические тени", "Shadows from dynamic lights",
         "Тени от динамических источников света"},
        {"effect_ssr", "SSR reflections", "Отражения SSR", "Screen-space reflections (not in the original)",
         "Экранные отражения (не было в оригинале)"},
        {"skip_intro", "Skip intros", "Пропуск заставок", "Skip the startup logos and intro",
         "Пропуск логотипов и заставки при запуске", false},
        {"debug_camera", "Free camera", "Свободная камера", "Toggled with Cross + L3",
         "Включается Cross + L3", false},
        {"debug_menu", "Debug menu", "Debug menu", "The game's debug menu (requires font files)",
         "Отладочное меню игры (нужны файлы шрифтов)", false},
    };
    for (int e = 0; e < BbSettings::EffectCount; ++e) {
        const auto& effect = BbSettings::Effects[e];
        const Short* name = nullptr;
        for (const Short& s : shorts) {
            if (std::strcmp(s.key, effect.key) == 0) name = &s;
        }
        if ((name && !name->graphics) || pages.back().rows.size() == pages.back().layout->rows) {
            continue; // the overlay menu and the launcher have it
        }
        // A new effect without a short name: its full label.
        const bool live = BbSettings::IsHotEffect(e);
        const std::string help_en = std::string(name ? name->help_en : effect.label) + (live ? "" : " (after a restart)");
        const std::string help_ru =
            std::string(name ? name->help_ru : effect.label_ru) + (live ? "" : " (после перезапуска)");
        row(Field(FirstEffect + e), name ? name->en : effect.label, name ? name->ru : effect.label_ru,
            help_en.c_str(), help_ru.c_str(), toggle);
    }

    // Unified System -> Controls window: Row 0 switches sections in-place, Rows 1..5 are the
    // 5 dynamic slots for the selected section.
    controls_rows.push_back(
        {ControlsSection,
         AddText("Section", "Раздел"),
         AddText("Use Left/Right or Confirm to switch between Combat, Actions, Weapons & Mouse, and Camera",
                 "Влево/Вправо или ОК: переключение между клавишами Боя, Действий, Оружия и Камерой"),
         {AddText("Keybinds: Combat (1/4)", "Клавиши: Бой (1/4)"),
          AddText("Keybinds: Actions (2/4)", "Клавиши: Действия (2/4)"),
          AddText("Keybinds: Weapons & Mouse (3/4)", "Клавиши: Оружие и мышь (3/4)"),
          AddText("Camera & Target (4/4)", "Камера и прицел (4/4)")},
         Kind::Select});
    for (int slot = 0; slot < ControlsSlotCount; ++slot) {
        controls_rows.push_back(
            {Field(FirstSlot + slot),
             ControlsSlotName + u32(slot),
             ControlsSlotHelp + u32(slot),
             {ControlsSlotVal + u32(slot), ControlsSlotVal + u32(slot)},
             Kind::Select});
    }
}

// ---- The game's structures, as its own code builds them. ----
constexpr std::size_t TextSize = 0x40;     // text entry (MakeText): text pointer, string, flag
constexpr std::size_t ValueSize = 0x48;    // {u32 value; text}
constexpr std::size_t ListSize = 0x920;    // 32 values + count at +0x908 (+ alignment slack)
constexpr std::size_t PairListSize = 0xb0; // 2 values + count at +0x98 (+ alignment slack)
constexpr std::size_t ListCountOffset = 0x908, PairCountOffset = 0x98;
constexpr std::size_t FunctionSize = 0x28; // std::function: 32-byte buffer, __f_ at +0x20

/// Destroys a std::function the way the game's code does (its __f_ at +0x20).
void DestroyFunction(u8* f) {
    void* impl = *reinterpret_cast<void**>(f + 0x20);
    if (!impl) {
        return;
    }
    using Destroy = void (PS4_SYSV_ABI*)(void*, int);
    Destroy destroy = (*reinterpret_cast<Destroy**>(impl))[4];
    destroy(impl, impl != f);
    *reinterpret_cast<void**>(f + 0x20) = nullptr;
}

/// Frees a text entry's string the way the game's code does (heap buffer once capacity >= 8).
void DestroyText(u8* text) {
    if (*reinterpret_cast<u64*>(text + 0x28) >= 8) {
        void* allocator = *reinterpret_cast<void**>(text + 0x30);
        using Free = void (PS4_SYSV_ABI*)(void*, void*);
        (*reinterpret_cast<Free**>(allocator))[0x70 / 8](allocator, *reinterpret_cast<void**>(text + 0x10));
    }
    *reinterpret_cast<u64*>(text + 0x28) = 7;
    *reinterpret_cast<u64*>(text + 0x20) = 0;
    *reinterpret_cast<char16_t*>(text + 0x10) = 0;
}
void DestroyTexts(u8* pair) {
    DestroyText(pair);
    DestroyText(pair + TextSize);
}
/// A value list's entries (count at `count_offset`), after the row has copied them.
void DestroyValues(u8* list, std::size_t count_offset) {
    const u64 count = *reinterpret_cast<u64*>(list + count_offset);
    for (u64 i = 0; i < count; ++i) {
        DestroyText(list + i * ValueSize + 8);
    }
}

void MakeTexts(u8* pair, u32 name, u32 help) {
    using Make = void* (PS4_SYSV_ABI*)(void*, u32, u32);
    Game<Make>(MakeText)(pair, TextName, name);
    Game<Make>(MakeText)(pair + TextSize, TextHelp, help);
}

/// Upscalers this GPU and build can run (known once the device exists, before any menu).
bool UpscalerAvailable(int i) {
    const auto& s = BbSettings::Get();
    return i == BbSettings::UpscalerFsr4     ? s.fsr4_supported.load()
           : i == BbSettings::UpscalerFsr411 ? s.fsr411_supported.load()
           : i == BbSettings::UpscalerDlss   ? s.dlss_supported.load()
                                             : true;
}

/// Another aspect ratio chosen on the page: the resolution row gets that aspect ratio's outputs
/// as its values (as many; the texts are looked up in the port's buffers, RefreshOutputs, when the
/// row shows them) and keeps the chosen height.
void RefreshOutputRow(u8* row) {
    const int aspect = std::clamp(values[Aspect], 0, BbSettings::AspectCount - 1);
    if (aspect == output_row_aspect) return;
    const BbSettings::OutputSize shown = MenuOutput(output_row_aspect, values[OutputRes]);
    output_row_aspect = aspect;
    menu_outputs = BbSettings::Outputs(aspect);
    RefreshOutputs(aspect);
    u8* items = row + RowValues;
    items += (0 - reinterpret_cast<std::uintptr_t>(items)) & 7;
    u64& count = *reinterpret_cast<u64*>(row + RowCount);
    // More values: copies of the first (its text an empty in-place string, see SelectRow) with
    // their own value and text.
    const u64 want = u64(menu_outputs.count);
    const bool in_place = *reinterpret_cast<u64*>(items + 0x30) < 8;
    if (count > 0 && want <= 32 && (want <= count || in_place)) {
        for (u64 i = count; i < want; ++i) {
            std::memcpy(items + i * ValueSize, items, ValueSize);
        }
        for (u64 i = 0; i < want; ++i) {
            *reinterpret_cast<u32*>(items + i * ValueSize) = u32(i);
            *reinterpret_cast<const char16_t**>(items + i * ValueSize + 8) = TextOf(OutputTexts + u32(i));
        }
        count = want;
        Game<void (PS4_SYSV_ABI*)(void*, u32)>(ListSetCount)(row + RowList, u32(want));
    }
    const int value = MenuOutputIndex(aspect, BbSettings::NearestOutput(aspect, shown));
    values[OutputRes] = value;
    default_values[OutputRes] = MenuOutputIndex(aspect, BbSettings::NearestOutput(aspect, BbSettings::OutputDefault));
    *reinterpret_cast<int*>(row + RowDefault) = default_values[OutputRes];
    Game<int (PS4_SYSV_ABI*)(void*, u32)>(ListSelect)(row + RowList, u32(value));
    Game<void (PS4_SYSV_ABI*)(void*)>(ListShow)(row + RowList);
    Game<void (PS4_SYSV_ABI*)(void*)>(RowShowValue)(row);
    Game<void (PS4_SYSV_ABI*)(void*)>(ListDraw)(row + RowList);
}

void AddRows(void* page, const std::vector<Row>& rows) {
    using Push = void (PS4_SYSV_ABI*)(void*, void*);
    using AddRow = void (PS4_SYSV_ABI*)(void*, void*, int*, void*, const int*);
    using Make = void* (PS4_SYSV_ABI*)(void*, u32, u32);
    for (const Row& row : rows) {
        alignas(16) u8 pair[2 * TextSize] = {};
        MakeTexts(pair, row.name, row.help);
        if (row.kind == Kind::Slider) {
            using Slider = void (PS4_SYSV_ABI*)(void*, void*, u8*, const u8*);
            Game<Slider>(SliderRow)(page, pair, &bytes[row.field], &default_bytes[row.field]);
            DestroyTexts(pair);
            continue;
        }
        if (row.kind == Kind::Toggle) {
            using PairRowFn = void (PS4_SYSV_ABI*)(void*, void*, u8*, void*, const u8*);
            u8* two = static_cast<u8*>(::operator new(PairListSize, std::align_val_t{16}));
            std::memset(two, 0, PairListSize);
            for (std::size_t i = 0; i < 2; ++i) {
                alignas(16) u8 value[ValueSize] = {};
                value[0] = u8(i);
                Game<Make>(MakeText)(value + 8, TextName, row.choices[i]);
                Game<Push>(PairPush)(two, value);
                DestroyText(value + 8);
            }
            Game<PairRowFn>(PairRow)(page, pair, &bytes[row.field], two, &default_bytes[row.field]);
            DestroyValues(two, PairCountOffset);
            ::operator delete(two, std::align_val_t{16});
            DestroyTexts(pair);
            continue;
        }
        u8* list = static_cast<u8*>(::operator new(ListSize, std::align_val_t{16}));
        std::memset(list, 0, ListSize);
        // The resolution row: as many as the aspect ratio has (its texts are rewritten).
        const std::size_t count =
            row.field == OutputRes ? std::size_t(menu_outputs.count) : row.choices.size();
        for (std::size_t i = 0; i < count; ++i) {
            if (row.field == Upscaler && !UpscalerAvailable(int(i))) {
                continue; // the list's values are the upscaler numbers, not positions
            }
            alignas(16) u8 value[ValueSize] = {};
            *reinterpret_cast<u32*>(value) = u32(i);
            Game<Make>(MakeText)(value + 8, TextName, row.choices[i]);
            Game<Push>(ListPush)(list, value);
            DestroyText(value + 8);
        }
        Game<AddRow>(row.kind == Kind::Select ? SelectRow : ListRow)(page, pair, &values[row.field], list,
                                                                    &default_values[row.field]);
        DestroyValues(list, ListCountOffset);
        ::operator delete(list, std::align_val_t{16});
        DestroyTexts(pair);
    }
}

std::atomic<int> building_page{-1}; // while the game builds one of our pages (its menu thread)
// Called by the game's menu code (guest threads, System V ABI like the game), one set per page.
template <int P>
void PS4_SYSV_ABI Rows(void* page, void*) {
    LoadValues();
    AddRows(page, pages[P].rows);
}
template <int P>
void* PS4_SYSV_ABI Content(void* a, void* b) {
    using Open = void* (PS4_SYSV_ABI*)(void*, void*, const char*, void*, u64, u64);
    void* page = Game<Open>(OpenPage)(a, b, Game<const char*>(pages[P].layout->name),
                                      reinterpret_cast<void*>(&Rows<P>), 0, 0);
    building_page = -1; // the layout's texts were looked up
    return page;
}
void* Factory(void* out, void* arg, void* content, int page) {
    using Create = void* (PS4_SYSV_ABI*)(void*, void*, void*);
    alignas(16) u8 function[FunctionSize] = {};
    *reinterpret_cast<u64*>(function) = image_base + ContentVtable;
    *reinterpret_cast<void**>(function + 8) = content;
    *reinterpret_cast<void**>(function + 0x20) = function;
    // The page is built later (the game's menu task): its layout title is looked up right before
    // the content function runs, which takes the override back.
    building_page = page;
    Game<Create>(CreatePage)(out, arg, function);
    DestroyFunction(function);
    return out;
}
template <int P>
void* PS4_SYSV_ABI PageFactory(void* out, void* arg) {
    return Factory(out, arg, reinterpret_cast<void*>(&Content<P>), P);
}
constexpr std::size_t MaxPages = 6;
void* const factories[MaxPages] = {
    reinterpret_cast<void*>(&PageFactory<0>), reinterpret_cast<void*>(&PageFactory<1>),
    reinterpret_cast<void*>(&PageFactory<2>), reinterpret_cast<void*>(&PageFactory<3>),
    reinterpret_cast<void*>(&PageFactory<4>), reinterpret_cast<void*>(&PageFactory<5>)};

// ---- Unified System -> Controls Window (Keybinds + Mouse + Camera/Controller) ----
enum class CapturePhase { WaitInitialRelease, WaitPressAndRelease, WaitEscapeRelease };
std::atomic<int> capturing_action{-1};
int capturing_slot = -1;
CapturePhase capture_phase = CapturePhase::WaitInitialRelease;
std::vector<std::string> captured_tokens;
std::string last_capture_preview;
std::atomic<long long> capture_cooldown_ms{0};
std::atomic<long long> last_capture_step_ms{0};

u8* active_controls_page = nullptr;
u8* active_game_settings = nullptr;
u8* slot_rows[ControlsSlotCount] = {};
int shown_controls_section = 0;

long long NowSteadyMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

bool IsModifierToken(const std::string& tok) {
    return tok == "Left Shift" || tok == "Right Shift" || tok == "Left Ctrl" ||
           tok == "Right Ctrl" || tok == "Left Alt" || tok == "Right Alt";
}

std::vector<std::string> CollectDownTokens(bool* escape_down) {
    std::vector<std::string> out;
    if (escape_down) *escape_down = false;
    if (!SDL_WasInit(SDL_INIT_VIDEO)) return out;
    int num_keys = 0;
    const bool* keys = SDL_GetKeyboardState(&num_keys);
    if (keys) {
        if (num_keys > SDL_SCANCODE_ESCAPE && keys[SDL_SCANCODE_ESCAPE]) {
            if (escape_down) *escape_down = true;
        }
        for (int sc = 4; sc < std::min(num_keys, int(SDL_SCANCODE_COUNT)); ++sc) {
            if (sc == SDL_SCANCODE_ESCAPE || !keys[sc]) continue;
            const char* raw_name = SDL_GetScancodeName(SDL_Scancode(sc));
            if (!raw_name || !*raw_name) continue;
            std::string name = raw_name;
            if (name == "Return") name = "Enter";
            out.push_back(std::move(name));
        }
    }
    const SDL_MouseButtonFlags mb = SDL_GetMouseState(nullptr, nullptr);
    if (mb & SDL_BUTTON_LMASK) out.emplace_back("Mouse Left");
    if (mb & SDL_BUTTON_RMASK) out.emplace_back("Mouse Right");
    if (mb & SDL_BUTTON_MMASK) out.emplace_back("Mouse Middle");
    if (mb & SDL_BUTTON_X1MASK) out.emplace_back("Mouse 4");
    if (mb & SDL_BUTTON_X2MASK) out.emplace_back("Mouse 5");
    return out;
}

void PlayMenuClickSound() {
    if (*reinterpret_cast<int*>(image_base + 0x557fc64) <= 0) {
        *reinterpret_cast<u64*>(image_base + 0x557fc60) = 0x3e8;
        *reinterpret_cast<u64*>(image_base + 0x557fc68) = image_base + 0x4933d56;
    }
}

/// Returns keybind action index (0..11) for (section, slot), or -1 if it's a numeric/toggle setting.
int SlotKeyAction(int section, int slot) {
    if (section == 0) return slot;      // 0..4: R1, R2, L2, L1, Dodge
    if (section == 1) return 5 + slot;  // 5..9: Sprint, Heal, UseItem, Interact, LockOn
    if (section == 2 && slot < 3) return 10 + slot; // 10..11: SwitchRight, SwitchLeft, 12(->11): Gestures
    return -1;
}

int CurrentMouseSpeedStep() {
    const auto& s = BbSettings::Get();
    return s.mouse_look
               ? std::clamp(int(std::lround(s.mouse_sensitivity / MouseSensStep)), 1, MouseSensSteps - 1)
               : 0;
}

void SetMouseSpeedStep(int step) {
    auto& s = BbSettings::Get();
    step = std::clamp(step, 0, MouseSensSteps - 1);
    s.mouse_look = step != 0;
    if (step != 0) {
        s.mouse_sensitivity = step * MouseSensStep;
    }
    BbSettings::Save();
}

void RefreshSlotTexts(int section, int slot) {
    const u32 name_id = ControlsSlotName + u32(slot);
    const u32 help_id = ControlsSlotHelp + u32(slot);
    const u32 val_id = ControlsSlotVal + u32(slot);

    const int k = SlotKeyAction(section, slot);
    if (k >= 0 && k < BbSettings::KeyActionCount) {
        const auto& def = BbSettings::GetKeyActionDef(k);
        SetDynamic(name_id, def.name_en, def.name_ru);
        SetDynamic(help_id, def.help_en, def.help_ru);
        const std::string raw = BbSettings::GetKeyBinding(k);
        const std::string en = BbSettings::FormatKeyBinding(raw, false);
        const std::string ru = BbSettings::FormatKeyBinding(raw, true);
        SetDynamic(val_id, en.c_str(), ru.c_str());
        return;
    }

    if (section == 2 && slot == 3) {
        SetDynamic(name_id, "Mouse Speed", "Чувствит. мыши");
        SetDynamic(help_id,
                   "Mouse camera sensitivity (Left/Right): 0 mouse look off, 5 default (1.0x), 10 fast (2.0x)",
                   "Чувствительность камеры мыши (Влево/Вправо): 0 выкл, 5 обычная (1.0x), 10 быстрая (2.0x)");
        const int step = CurrentMouseSpeedStep();
        if (step == 0) {
            SetDynamic(val_id, "Off", "Выкл");
        } else {
            const std::string txt = fmt::format("{} / 10 ({:.1f}x)", step, step * MouseSensStep);
            SetDynamic(val_id, txt.c_str(), txt.c_str());
        }
        return;
    }

    if (section == 2 && slot == 4) {
        SetDynamic(name_id, "Controller Vibration", "Вибрация геймпада");
        SetDynamic(help_id,
                   "Controller vibration strength (Left/Right): 0 off, 10 strongest",
                   "Сила вибрации геймпада (Влево/Вправо): 0 выкл, 10 макс.");
        const int vib = active_game_settings ? std::clamp(int(active_game_settings[0x8]), 0, 10) : 10;
        if (vib == 0) {
            SetDynamic(val_id, "Off (0 / 10)", "Выкл (0 / 10)");
        } else {
            const std::string txt = fmt::format("{} / 10", vib);
            SetDynamic(val_id, txt.c_str(), txt.c_str());
        }
        return;
    }

    // Section 3: Camera & Target (Bloodborne's native Controls settings at active_game_settings)
    if (slot == 0) {
        SetDynamic(name_id, "Camera X-Axis", "Камера по оси X");
        SetDynamic(help_id, "Horizontal camera rotation direction (Left/Right to toggle)",
                   "Направление вращения камеры по горизонтали (Влево/Вправо)");
        const bool inv = active_game_settings && active_game_settings[0x9] != 0;
        SetDynamic(val_id, inv ? "Inverted" : "Normal", inv ? "Инверсия" : "Обычная");
    } else if (slot == 1) {
        SetDynamic(name_id, "Camera Y-Axis", "Камера по оси Y");
        SetDynamic(help_id, "Vertical camera rotation direction (Left/Right to toggle)",
                   "Направление вращения камеры по вертикали (Влево/Вправо)");
        const bool inv = active_game_settings && active_game_settings[0xa] != 0;
        SetDynamic(val_id, inv ? "Inverted" : "Normal", inv ? "Инверсия" : "Обычная");
    } else if (slot == 2) {
        SetDynamic(name_id, "Camera Sensitivity", "Скорость камеры");
        SetDynamic(help_id, "Gamepad & lock-on camera rotation speed (Left/Right: 0 .. 10)",
                   "Скорость вращения камеры (Влево/Вправо: 0 .. 10)");
        const int sens = active_game_settings ? std::clamp(int(active_game_settings[0xb]), 0, 10) : 5;
        const std::string txt = fmt::format("{} / 10", sens);
        SetDynamic(val_id, txt.c_str(), txt.c_str());
    } else if (slot == 3) {
        SetDynamic(name_id, "Auto Target Switch", "Автосмена цели");
        SetDynamic(help_id, "Automatically switch lock-on target when current target is defeated",
                   "Автоматическое переключение захвата цели после гибели текущей цели");
        const bool on = active_game_settings && active_game_settings[0x6] != 0;
        SetDynamic(val_id, on ? "On" : "Off", on ? "Вкл" : "Выкл");
    } else if (slot == 4) {
        SetDynamic(name_id, "Camera Auto-Adjust", "Автокоррекция камеры");
        SetDynamic(help_id, "Automatically adjust camera when colliding with walls",
                   "Автоматический обход стен при движении камеры");
        const bool on = !active_game_settings || active_game_settings[0x7] != 0;
        SetDynamic(val_id, on ? "On" : "Off", on ? "Вкл" : "Выкл");
    }
}

void UpdateSlotRowDisplay(u8* row, int slot) {
    if (!row) return;
    u8* items = row + RowValues;
    items += (0 - reinterpret_cast<std::uintptr_t>(items)) & 7;
    const u64 count = *reinterpret_cast<u64*>(row + RowCount);
    for (u64 i = 0; i < count && i < 2; ++i) {
        *reinterpret_cast<const char16_t**>(items + i * ValueSize + 8) = TextOf(ControlsSlotVal + u32(slot));
    }
    Game<void (PS4_SYSV_ABI*)(void*)>(RowShowValue)(row);
}

void RefreshControlsPage() {
    shown_controls_section = std::clamp(values[ControlsSection], 0, ControlsSectionCount - 1);
    for (int slot = 0; slot < ControlsSlotCount; ++slot) {
        RefreshSlotTexts(shown_controls_section, slot);
    }
    if (active_controls_page) {
        u8* const rows_begin = *reinterpret_cast<u8**>(active_controls_page + 0xe58);
        u8* const rows_end = *reinterpret_cast<u8**>(active_controls_page + 0xe60);
        if (rows_begin && rows_end && rows_end - rows_begin >= (1 + ControlsSlotCount) * 0x90) {
            for (int slot = 0; slot < ControlsSlotCount; ++slot) {
                u8* const entry = rows_begin + (1 + slot) * 0x90;
                *reinterpret_cast<const char16_t**>(entry + 0x00) = TextOf(ControlsSlotName + u32(slot));
                *reinterpret_cast<const char16_t**>(entry + 0x40) = TextOf(ControlsSlotHelp + u32(slot));
                u8* const widget = *reinterpret_cast<u8**>(entry + 0x80);
                if (widget) {
                    slot_rows[slot] = widget;
                }
            }
        }
        Game<void (PS4_SYSV_ABI*)(void*)>(ListDraw)(active_controls_page + 0xa70);
        Game<void (PS4_SYSV_ABI*)(void*)>(UpdatePageHelpBar)(active_controls_page);
    }
    for (int slot = 0; slot < ControlsSlotCount; ++slot) {
        UpdateSlotRowDisplay(slot_rows[slot], slot);
    }
}

void StepSlotSetting(u8* row, int section, int slot, int delta, bool wrap) {
    if (section == 2 && slot == 3) {
        int step = CurrentMouseSpeedStep() + delta;
        step = wrap ? (step + MouseSensSteps) % MouseSensSteps : std::clamp(step, 0, MouseSensSteps - 1);
        SetMouseSpeedStep(step);
    } else if (section == 2 && slot == 4 && active_game_settings) {
        int vib = int(active_game_settings[0x8]) + delta;
        vib = wrap ? (vib + 11) % 11 : std::clamp(vib, 0, 10);
        active_game_settings[0x8] = u8(vib);
    } else if (section == 3 && active_game_settings) {
        if (slot == 0) {
            active_game_settings[0x9] = active_game_settings[0x9] ? 0 : 1;
        } else if (slot == 1) {
            active_game_settings[0xa] = active_game_settings[0xa] ? 0 : 1;
        } else if (slot == 2) {
            int sens = int(active_game_settings[0xb]) + delta;
            sens = wrap ? (sens + 11) % 11 : std::clamp(sens, 0, 10);
            active_game_settings[0xb] = u8(sens);
        } else if (slot == 3) {
            active_game_settings[0x6] = active_game_settings[0x6] ? 0 : 1;
        } else if (slot == 4) {
            active_game_settings[0x7] = active_game_settings[0x7] ? 0 : 1;
        }
    }
    PlayMenuClickSound();
    RefreshSlotTexts(section, slot);
    UpdateSlotRowDisplay(row, slot);
}

void StepKeyCapture(u8* row, int slot, int k) {
    last_capture_step_ms.store(NowSteadyMs(), std::memory_order_relaxed);
    bool escape_down = false;
    const std::vector<std::string> down = CollectDownTokens(&escape_down);

    if (capture_phase == CapturePhase::WaitInitialRelease) {
        if (!escape_down && down.empty()) {
            capture_phase = CapturePhase::WaitPressAndRelease;
        }
        return;
    }
    if (capture_phase == CapturePhase::WaitEscapeRelease) {
        if (!escape_down && down.empty()) {
            RefreshSlotTexts(shown_controls_section, slot);
            UpdateSlotRowDisplay(row, slot);
            capture_cooldown_ms.store(NowSteadyMs() + 200, std::memory_order_relaxed);
            capturing_action.store(-1, std::memory_order_relaxed);
        }
        return;
    }
    if (escape_down) {
        capture_phase = CapturePhase::WaitEscapeRelease;
        return;
    }
    for (const std::string& tok : down) {
        if (captured_tokens.size() < 3 &&
            std::find(captured_tokens.begin(), captured_tokens.end(), tok) == captured_tokens.end()) {
            captured_tokens.push_back(tok);
        }
    }
    if (!down.empty() && !captured_tokens.empty()) {
        std::string preview = "[ ";
        for (std::size_t i = 0; i < captured_tokens.size(); ++i) {
            if (i > 0) preview += " + ";
            preview += captured_tokens[i];
        }
        preview += " ... ]";
        if (preview != last_capture_preview) {
            last_capture_preview = preview;
            SetDynamic(ControlsSlotVal + u32(slot), preview.c_str(), preview.c_str());
            UpdateSlotRowDisplay(row, slot);
        }
        return;
    }
    if (down.empty() && !captured_tokens.empty()) {
        // Order modifiers first so chords like Left Shift + Mouse Left are canonical.
        std::stable_partition(captured_tokens.begin(), captured_tokens.end(), IsModifierToken);
        std::string new_binding;
        for (std::size_t i = 0; i < captured_tokens.size(); ++i) {
            if (i > 0) new_binding += "+";
            new_binding += captured_tokens[i];
        }
        BbSettings::SetKeyBinding(k, new_binding);
        BbSettings::Save();
        RefreshSlotTexts(shown_controls_section, slot);
        UpdateSlotRowDisplay(row, slot);
        PlayMenuClickSound();
        capture_cooldown_ms.store(NowSteadyMs() + 200, std::memory_order_relaxed);
        capturing_action.store(-1, std::memory_order_relaxed);
    }
}

// ---- Hooks ----
using ControlsContentFn = void (PS4_SYSV_ABI*)(void*, u8*);
ControlsContentFn controls_content_original = nullptr;

void PS4_SYSV_ABI ControlsContentHook(void* page, u8* game_settings) {
    active_controls_page = static_cast<u8*>(page);
    active_game_settings = game_settings;
    for (int slot = 0; slot < ControlsSlotCount; ++slot) {
        slot_rows[slot] = nullptr;
    }
    LoadValues();
    shown_controls_section = std::clamp(values[ControlsSection], 0, ControlsSectionCount - 1);
    for (int slot = 0; slot < ControlsSlotCount; ++slot) {
        RefreshSlotTexts(shown_controls_section, slot);
    }
    AddRows(page, controls_rows);
    u8* const rows_begin = *reinterpret_cast<u8**>(active_controls_page + 0xe58);
    u8* const rows_end = *reinterpret_cast<u8**>(active_controls_page + 0xe60);
    if (rows_begin && rows_end && rows_end - rows_begin >= (1 + ControlsSlotCount) * 0x90) {
        for (int slot = 0; slot < ControlsSlotCount; ++slot) {
            slot_rows[slot] = *reinterpret_cast<u8**>(rows_begin + (1 + slot) * 0x90 + 0x80);
        }
    }
}

using RowInputFn = u64 (PS4_SYSV_ABI*)(u8*, u64, u64, u64, u64, u64, double);
RowInputFn row_input_original = nullptr;

u64 PS4_SYSV_ABI RowInputHook(u8* row, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, double frame_time) {
    int* const val_ptr = *reinterpret_cast<int**>(row + RowValue);
    // The game asks the open page's rows: the resolution row is alive while it does.
    if (val_ptr == &values[OutputRes]) {
        RefreshOutputRow(row);
    }
    if (val_ptr == &values[ControlsSection]) {
        const u8 in = u8(a1);
        if (row[0x3c0] == 0 && in != 0) {
            const bool left = Game<bool (PS4_SYSV_ABI*)(const u8*)>(IsLeftPressed)(&in);
            const bool right = !left && Game<bool (PS4_SYSV_ABI*)(const u8*)>(IsRightPressed)(&in);
            if (left || right) {
                const int dir = left ? -1 : 1;
                values[ControlsSection] =
                    (values[ControlsSection] + ControlsSectionCount + dir) % ControlsSectionCount;
                applied[ControlsSection] = values[ControlsSection];
                Game<int (PS4_SYSV_ABI*)(void*, u32)>(ListSelect)(row + RowList, u32(values[ControlsSection]));
                Game<void (PS4_SYSV_ABI*)(void*)>(ListShow)(row + RowList);
                Game<void (PS4_SYSV_ABI*)(void*)>(RowShowValue)(row);
                PlayMenuClickSound();
                RefreshControlsPage();
                return 0;
            }
        }
        const u64 res = row_input_original(row, a1, a2, a3, a4, a5, frame_time);
        if (values[ControlsSection] != shown_controls_section) {
            applied[ControlsSection] = values[ControlsSection];
            RefreshControlsPage();
        }
        return res;
    }
    if (val_ptr >= &values[FirstSlot] && val_ptr < &values[int(FirstSlot) + ControlsSlotCount]) {
        const int slot = int(val_ptr - &values[FirstSlot]);
        slot_rows[slot] = row;
        // If the page's "Defaults" button was pressed, it sets values[FirstSlot + slot] = 1.
        if (values[FirstSlot + slot] != 0) {
            for (int s = 0; s < ControlsSlotCount; ++s) {
                values[FirstSlot + s] = 0;
                applied[FirstSlot + s] = 0;
            }
            for (int k = 0; k < BbSettings::KeyActionCount; ++k) {
                BbSettings::SetKeyBinding(k, BbSettings::GetKeyActionDef(k).default_val);
            }
            SetMouseSpeedStep(5);
            if (active_game_settings) {
                active_game_settings[0x6] = 0;
                active_game_settings[0x7] = 1;
                active_game_settings[0x8] = 10;
                active_game_settings[0x9] = 0;
                active_game_settings[0xa] = 0;
                active_game_settings[0xb] = 5;
            }
            RefreshControlsPage();
        }
        const int k = SlotKeyAction(shown_controls_section, slot);
        if (k >= 0) {
            const int active_k = capturing_action.load(std::memory_order_relaxed);
            if (active_k == k && capturing_slot == slot) {
                StepKeyCapture(row, slot, k);
                return 0;
            }
            if (active_k < 0 && NowSteadyMs() >= capture_cooldown_ms.load(std::memory_order_relaxed)) {
                const u8 in = u8(a1);
                if (in != 0 && Game<bool (PS4_SYSV_ABI*)(const u8*)>(IsConfirmPressed)(&in)) {
                    PlayMenuClickSound();
                    captured_tokens.clear();
                    last_capture_preview.clear();
                    capture_phase = CapturePhase::WaitInitialRelease;
                    last_capture_step_ms.store(NowSteadyMs(), std::memory_order_relaxed);
                    capturing_slot = slot;
                    capturing_action.store(k, std::memory_order_relaxed);
                    SetDynamic(ControlsSlotVal + u32(slot), "[ Press key(s)... ]", "[ Нажмите клавишу... ]");
                    UpdateSlotRowDisplay(row, slot);
                }
            }
            return 0;
        }
        // Setting row (Mouse Speed, Controller Vibration, or Camera & Target setting)
        const u8 in = u8(a1);
        if (in != 0 && capturing_action.load(std::memory_order_relaxed) < 0) {
            const bool left = Game<bool (PS4_SYSV_ABI*)(const u8*)>(IsLeftPressed)(&in);
            const bool right = !left && Game<bool (PS4_SYSV_ABI*)(const u8*)>(IsRightPressed)(&in);
            const bool confirm = !left && !right && Game<bool (PS4_SYSV_ABI*)(const u8*)>(IsConfirmPressed)(&in);
            if (left || right || confirm) {
                StepSlotSetting(row, shown_controls_section, slot, left ? -1 : 1, confirm);
            }
        }
        return 0;
    }
    return row_input_original(row, a1, a2, a3, a4, a5, frame_time);
}

using LookupFn = const char16_t* (PS4_SYSV_ABI*)(void*, u32, u32, u32);
using AddFn = void* (PS4_SYSV_ABI*)(void*, void*, void*, void*);
LookupFn lookup_original = nullptr;
AddFn add_original = nullptr;
std::atomic<const void*> screen_sound_text{nullptr};

const char16_t* PS4_SYSV_ABI LookupHook(void* repository, u32 table, u32 category, u32 id) {
    if (OurText(id)) {
        // BB_GAME_MENU_TRACE=1: how often the game asks for our texts (per 2 s).
        static const bool count = [] {
            const char* env = std::getenv("BB_GAME_MENU_TRACE");
            return env && env[0] == '1';
        }();
        if (count) {
            static std::atomic<u64> lookups{0};
            static std::atomic<long long> printed{0};
            ++lookups;
            const long long now = std::chrono::duration_cast<std::chrono::seconds>(
                                      std::chrono::steady_clock::now().time_since_epoch())
                                      .count();
            if (now - printed.load() >= 2) {
                printed = now;
                std::printf("Game menu: %llu lookups of the port's texts in 2 s (last id %u)\n",
                            (unsigned long long)lookups.exchange(0), id - IdBase);
            }
        }
        return TextOf(id);
    }
    if (const int p = building_page.load(); p >= 0 && category == TextName) {
        if (id == pages[p].layout->title) return TextOf(pages[p].name);
        if (pages[p].layout->subtitle && id == pages[p].layout->subtitle) return TextOf(pages[p].help);
    }
    const char16_t* text = lookup_original(repository, table, category, id);
    // BB_MSG_TRACE=1 (diagnostics): the game's text lookups per 2 s by category, with an example
    // id and caller and its text (which menu is on screen, issue #255).
    static const bool msg_trace = [] {
        const char* env = std::getenv("BB_MSG_TRACE");
        return env && env[0] == '1';
    }();
    if (msg_trace) {
        struct Seen {
            u64 count = 0;
            u32 id = 0;
            u64 caller = 0;
            std::string text;
        };
        static std::mutex mutex;
        static std::map<std::pair<u32, u32>, Seen> seen;
        static long long printed = 0;
        std::scoped_lock lock{mutex};
        auto& s = seen[{table, category}];
        if (s.count++ == 0) {
            s.id = id;
            s.caller = reinterpret_cast<u64>(__builtin_return_address(0)) - image_base;
            for (const char16_t* c = text; c && *c && s.text.size() < 40; ++c) {
                s.text += *c < 0x80 ? char(*c) : '?';
            }
        }
        const long long now = std::chrono::duration_cast<std::chrono::seconds>(
                                  std::chrono::steady_clock::now().time_since_epoch())
                                  .count();
        if (now - printed >= 2) {
            printed = now;
            std::string line;
            for (const auto& [key, v] : seen) {
                line += fmt::format(" [{}/{:#x}: {} e.g. {} from {:#x} \"{}\"]", key.first, key.second,
                                    v.count, v.id, v.caller, v.text);
            }
            std::printf("Msg trace (2 s):%s\n", line.c_str());
            seen.clear();
        }
    }
    static const bool trace = [] {
        const char* env = std::getenv("BB_GAME_MENU_TRACE");
        return env && env[0] == '1';
    }();
    if (trace && id >= 110000 && id < 120000) {
        std::printf("Game menu: text %#x/%u%s from %#llx\n", category, id, building_page >= 0 ? " (our page)" : "",
                    (unsigned long long)(reinterpret_cast<u64>(__builtin_return_address(0)) - image_base));
    }
    if (category == TextName && id == ScreenSoundItem && text) {
        screen_sound_text = text;
        bool cyrillic = false;
        for (const char16_t* c = text; *c; ++c) {
            cyrillic = cyrillic || (*c >= 0x400 && *c < 0x500);
        }
        game_language = cyrillic ? GameLanguage::Russian : GameLanguage::English;
    }
    return text;
}

void AddItem(void* menu, u32 name, u32 help, void* factory) {
    using ToFunction = void* (PS4_SYSV_ABI*)(void*, void*);
    alignas(16) u8 pair[2 * TextSize] = {};
    MakeTexts(pair, name, help);
    alignas(16) u8 source[FunctionSize] = {};
    *reinterpret_cast<u64*>(source) = image_base + FactoryVtable;
    *reinterpret_cast<void**>(source + 8) = factory;
    *reinterpret_cast<void**>(source + 0x20) = source;
    alignas(16) u8 function[FunctionSize] = {};
    *reinterpret_cast<void**>(function + 0x20) = Game<ToFunction>(FactoryToFunction)(source, function);
    alignas(16) u8 out[16] = {};
    add_original(menu, pair, function, out);
    DestroyFunction(function);
    DestroyTexts(pair);
}

void* PS4_SYSV_ABI AddHook(void* menu, void* pair, void* function, void* out) {
    void* result = add_original(menu, pair, function, out);
    const void* item_text = *reinterpret_cast<const void* const*>(pair);
    const void* screen_sound = screen_sound_text.load();
    // Place Display and Effects right under Environment / Screen & Sound (110001).
    // Controls (110000) itself now houses all Keybinds + Mouse + Camera settings in one window.
    if (screen_sound && item_text == screen_sound) {
        for (std::size_t i = 0; i < pages.size() && i < MaxPages; ++i) {
            AddItem(menu, pages[i].name, pages[i].help, factories[i]);
        }
    }
    return result;
}

/// `jmp hook` at the function (via a near thunk); its moved prologue + `jmp` back is the original.
bool Detour(unsigned char* image, u64 va, const u8* prologue, std::size_t length, void* hook,
            void** original, u8*& cursor) {
    u8* site = image + va;
    if (std::memcmp(site, prologue, length) != 0) {
        std::printf("Game menu: unexpected code at %#llx, not hooked\n", (unsigned long long)va);
        return false;
    }
    u8* trampoline = cursor;
    std::memcpy(trampoline, prologue, length);
    trampoline[length] = 0xe9;
    const std::int64_t back = std::int64_t(site + length) - std::int64_t(trampoline + length + 5);
    const std::int32_t back32 = std::int32_t(back);
    std::memcpy(trampoline + length + 1, &back32, 4);
    u8* thunk = trampoline + 32;
    thunk[0] = 0x48;
    thunk[1] = 0xb8; // movabs rax, hook
    std::memcpy(thunk + 2, &hook, 8);
    thunk[10] = 0xff;
    thunk[11] = 0xe0; // jmp rax
    const std::int64_t to = std::int64_t(thunk) - std::int64_t(site + 5);
    if (back != back32 || to != std::int32_t(to)) {
        std::printf("Game menu: stubs out of jump range, not hooked\n");
        return false;
    }
    const std::int32_t to32 = std::int32_t(to);
    site[0] = 0xe9;
    std::memcpy(site + 1, &to32, 4);
    for (std::size_t i = 5; i < length; ++i) {
        site[i] = 0xcc;
    }
    *original = trampoline;
    cursor += 64;
    return true;
}

/// A mapping within +-2 GiB of the image, for rel32 jumps both ways.
u8* MapNear(unsigned char* image, u64 image_size, std::size_t size) {
    const u64 base = reinterpret_cast<u64>(image);
    for (u64 k = 1; k <= 64; ++k) {
        for (const u64 hint : {base - k * (24ull << 20), base + image_size + k * (24ull << 20)}) {
            void* p = mmap(reinterpret_cast<void*>(hint), size, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
            if (p != MAP_FAILED) {
                return static_cast<u8*>(p);
            }
        }
    }
    return nullptr;
}

} // namespace

void PatchImage(unsigned char* image, std::uint64_t size) {
    if (const char* env = std::getenv("BB_GAME_MENU"); env && env[0] == '0') {
        return;
    }
    image_base = reinterpret_cast<u64>(image);
    const std::size_t page = std::size_t(sysconf(_SC_PAGESIZE));
    u8* stubs = MapNear(image, size, page);
    if (!stubs) {
        std::printf("Game menu: no memory near the image, the port's pages are off\n");
        return;
    }
    DefineTexts();
    for (const Page& p : pages) {
        for (const Row& r : p.rows) {
            byte_field[r.field] = r.kind == Kind::Toggle || r.kind == Kind::Slider;
        }
    }
    static const BbSettings::Values defaults;
    Fill(defaults, default_values);
    for (int e = 0; e < BbSettings::EffectCount; ++e) {
        default_values[FirstEffect + e] = BbSettings::Effects[e].default_on;
    }
    default_values[ControlsSection] = 0;
    for (int slot = 0; slot < ControlsSlotCount; ++slot) {
        default_values[FirstSlot + slot] = 1; // 1 triggers default reset when "Defaults" is clicked
    }
    for (int f = 0; f < FieldCount; ++f) {
        default_bytes[f] = u8(default_values[f]);
    }
    u8* cursor = stubs;
    // The lookup first: the menu hook only acts once it has seen the Screen/Sound text.
    const bool ok =
        Detour(image, MsgLookup, MsgLookupPrologue, sizeof(MsgLookupPrologue),
               reinterpret_cast<void*>(&LookupHook), reinterpret_cast<void**>(&lookup_original), cursor) &&
        Detour(image, AddPageItem, AddPageItemPrologue, sizeof(AddPageItemPrologue),
               reinterpret_cast<void*>(&AddHook), reinterpret_cast<void**>(&add_original), cursor) &&
        Detour(image, ControlsContent, AddPageItemPrologue, sizeof(AddPageItemPrologue),
               reinterpret_cast<void*>(&ControlsContentHook),
               reinterpret_cast<void**>(&controls_content_original), cursor) &&
        Detour(image, RowInput, RowInputPrologue, sizeof(RowInputPrologue),
               reinterpret_cast<void*>(&RowInputHook), reinterpret_cast<void**>(&row_input_original), cursor);
    mprotect(stubs, page, PROT_READ | PROT_EXEC);
    std::printf("Game menu: %s (unified Controls + %zu pages in System)\n",
                ok ? "the port's pages are in" : "hooks failed, off", pages.size());
}

bool CapturesInput() {
    const long long now = NowSteadyMs();
    if (capturing_action.load(std::memory_order_relaxed) >= 0) {
        if (now - last_capture_step_ms.load(std::memory_order_relaxed) > 400) {
            capturing_action.store(-1, std::memory_order_relaxed);
            return false;
        }
        return true;
    }
    return now < capture_cooldown_ms.load(std::memory_order_relaxed);
}

void Poll() {
    if (!values_valid) {
        return;
    }
    // A copy: the game's rows are bound to `values` (writing it would move them back).
    int now_values[FieldCount];
    std::memcpy(now_values, values, sizeof(now_values));
    for (int f = 0; f < FieldCount; ++f) {
        if (byte_field[f]) {
            now_values[f] = bytes[f];
        }
    }
    // The aspect ratio, the resolution and the preset re-create the scene targets and the
    // upscaler (a few grey frames): stepping through several did that at each one. Applied once
    // they rest for 0.4 s; the other rows at once.
    using Clock = std::chrono::steady_clock;
    static int resting[3] = {-1, -1, -1};
    static Clock::time_point rested_since{};
    const auto now = Clock::now();
    if (resting[0] != now_values[Aspect] || resting[1] != now_values[OutputRes] ||
        resting[2] != now_values[Preset]) {
        resting[0] = now_values[Aspect];
        resting[1] = now_values[OutputRes];
        resting[2] = now_values[Preset];
        rested_since = now;
    }
    if (now - rested_since < std::chrono::milliseconds(400)) {
        now_values[Aspect] = applied[Aspect];
        now_values[OutputRes] = applied[OutputRes];
        now_values[Preset] = applied[Preset];
    }
    if (std::memcmp(now_values, applied, sizeof(now_values)) == 0) {
        return;
    }
    auto& s = BbSettings::Get();
    if (now_values[Aspect] != applied[Aspect]) {
        // Another aspect ratio keeps the resolution row's height (its texts show the new
        // aspect ratio's sizes, RefreshOutputRow).
        const int aspect = std::clamp(now_values[Aspect], 0, BbSettings::AspectCount - 1);
        s.output_res = BbSettings::PackSize(MenuOutput(aspect, now_values[OutputRes]));
        s.aspect = aspect;
    } else if (now_values[OutputRes] != applied[OutputRes]) {
        s.output_res = BbSettings::PackSize(MenuOutput(s.aspect, now_values[OutputRes]));
    }
    const int upscaler = std::clamp(now_values[Upscaler], 0, BbSettings::UpscalerCount - 1);
    s.upscaler = UpscalerAvailable(upscaler) ? upscaler : int(BbSettings::UpscalerFsr3);
    s.preset = std::clamp(now_values[Preset], 0, BbSettings::PresetCount - 1);
    s.sharpen = now_values[Sharpness] != 0;
    if (now_values[Sharpness] != 0) {
        s.sharpness = std::clamp(now_values[Sharpness], 0, SharpnessSteps - 1) * SharpnessStep;
    }
    s.model_lod = LodValues[std::clamp(now_values[ModelLod], 0, 3)];
    for (int e = 0; e < BbSettings::EffectCount; ++e) {
        s.effects[e] = now_values[FirstEffect + e] != 0;
    }
    BbGuestHooks::ApplyGameEffects();
    std::memcpy(applied, now_values, sizeof(now_values));
    BbSettings::Save();
}

} // namespace BbGameMenu
