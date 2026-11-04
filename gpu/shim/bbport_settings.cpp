// SPDX-License-Identifier: GPL-2.0-or-later
#include "bbport_settings.h"
#include "bbport_frame_rate.h"
#include <cmath>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

extern "C" __attribute__((weak)) void runtime_pad_reload_bindings(void);

namespace BbSettings {

namespace {

constexpr KeyActionDef kKeyDefs[KeyActionCount] = {
    {"key.r1", "Attack (R1)", "Обычный удар (R1)",
     "Right-hand weapon light attack (R1)",
     "Быстрый удар оружием в правой руке (R1)",
     "Mouse Left"},
    {"key.r2", "Strong Attack (R2)", "Сильный удар (R2)",
     "Right-hand heavy / charged attack (R2). Hold from behind for backstab",
     "Сильная / заряженная атака (R2). Удерживайте со спины для оглушения",
     "Left Shift+Mouse Left, Mouse 4"},
    {"key.l2", "Firearm / Parry (L2)", "Выстрел / Парирование (L2)",
     "Left-hand firearm shot or two-handed L2 attack (parry enemy attacks)",
     "Выстрел из пистолета в левой руке или особая атака L2 (парирование)",
     "Mouse Right"},
    {"key.l1", "Transform Weapon (L1)", "Трансформация (L1)",
     "Trick weapon transformation (L1). After dodge or R1: transform attack",
     "Трансформация оружия-обманки (L1). После удара/уворота — атака с трансф.",
     "E, Mouse 5"},
    {"key.dodge", "Dodge / Backstep", "Уклонение / Отскок",
     "Quickstep / Roll (with WASD) or Backstep (standing still)",
     "Отскок / Перекат (с WASD) или шаг назад (стоя на месте)",
     "Space"},
    {"key.sprint", "Sprint & Jump (Hold)", "Бег и прыжок (удерж.)",
     "Hold to Sprint; press Dodge while sprinting to Jump",
     "Удерживайте для бега; нажмите Уклонение на бегу для прыжка",
     "Left Shift"},
    {"key.triangle", "Heal / Blood Vial", "Лечение (Пузырёк)",
     "Use Blood Vial to restore HP (Triangle)",
     "Использовать Пузырёк крови для лечения (Треугольник)",
     "Q"},
    {"key.square", "Use Quick Item", "Применить предмет",
     "Use selected quick item: Molotov, Bolt Paper, Hunter Tool (Square)",
     "Применить выбранный предмет: коктейль Молотова, бумага, инструмент",
     "R, Tab"},
    {"key.cross", "Interact / Confirm", "Действие / ОК",
     "Talk, open doors, pick up items, light lamps, confirm in menus (Cross)",
     "Говорить, открывать двери, подбирать предметы, зажигать фонари (Крест)",
     "F, Enter"},
    {"key.r3", "Lock-On Target (R3)", "Захват цели (R3)",
     "Lock camera onto enemy or reset camera forward (R3)",
     "Захват цели камерой или центрирование камеры (R3)",
     "C, Mouse Middle"},
    {"key.down", "Switch Quick Item (Down)", "Смена предмета (Вниз)",
     "Cycle through equipped quick items / Hunter Tools (D-Pad Down)",
     "Переключение выбранного быстрого предмета / инструмента (Вниз)",
     "Wheel Down, 4"},
    {"key.up", "Blood Bullets (Up)", "Кровавые пули (Вверх)",
     "Create +5 emergency Quicksilver Bullets using HP (D-Pad Up)",
     "Создать +5 экстренных ртутных пуль за счёт здоровья (Вверх)",
     "Wheel Up, 3"},
    {"key.right", "Switch Right Weapon", "Смена правого оружия",
     "Cycle between right-hand trick weapons (D-Pad Right)",
     "Переключение оружия в правой руке (Крестовина вправо)",
     "2"},
    {"key.left", "Switch Left Weapon", "Смена левого оружия",
     "Cycle between left-hand firearms / torches (D-Pad Left)",
     "Переключение пистолета / факела в левой руке (Крестовина влево)",
     "1"},
    {"key.touchpad", "Gestures (Left Pad)", "Жесты (Левый тачпад)",
     "Open Gestures menu (Left side of Touchpad)",
     "Открыть меню жестов (Левая сторона тачпада)",
     "G"},
    {"key.touchpad_right", "Personal Effects (Right)", "Личные вещи (Правый)",
     "Open Personal Effects quick belt (Right side of Touchpad)",
     "Быстрое меню личных вещей (Правая сторона тачпада)",
     "Backspace"},
    {"key.l3", "Jump / Sort (L3)", "Прыжок / Сортировка (L3)",
     "Left Stick Click (L3): sort inventory in menus, or L3 jump",
     "Нажатие левого стика (L3): сортировка инвентаря или прыжок",
     "Left Ctrl"},
    {"key.options", "System Menu (Options)", "Меню системы (Options)",
     "Open main equipment, inventory, and system menu (Options)",
     "Открыть главное меню экипировки, инвентаря и системы (Options)",
     "Escape"},
    {"key.move_up", "Move Forward", "Движение вперёд",
     "Move character forward (Left Stick Up)",
     "Движение персонажа вперёд (Левый стик вверх)",
     "W"},
    {"key.move_down", "Move Backward", "Движение назад",
     "Move character backward (Left Stick Down)",
     "Движение персонажа назад (Левый стик вниз)",
     "S"},
    {"key.move_left", "Move Left", "Движение влево",
     "Strafe / move character left (Left Stick Left)",
     "Движение персонажа влево (Левый стик влево)",
     "A"},
    {"key.move_right", "Move Right", "Движение вправо",
     "Strafe / move character right (Left Stick Right)",
     "Движение персонажа вправо (Левый стик вправо)",
     "D"},
    {"key.look_up", "Camera Up (Key)", "Камера вверх (Кл.)",
     "Rotate camera up using keyboard (Right Stick Up)",
     "Поворот камеры вверх с клавиатуры (Правый стик вверх)",
     "I"},
    {"key.look_down", "Camera Down (Key)", "Камера вниз (Кл.)",
     "Rotate camera down using keyboard (Right Stick Down)",
     "Поворот камеры вниз с клавиатуры (Правый стик вниз)",
     "K"},
    {"key.look_left", "Camera Left (Key)", "Камера влево (Кл.)",
     "Rotate camera left using keyboard (Right Stick Left)",
     "Поворот камеры влево с клавиатуры (Правый стик влево)",
     "J"},
    {"key.look_right", "Camera Right (Key)", "Камера вправо (Кл.)",
     "Rotate camera right using keyboard (Right Stick Right)",
     "Поворот камеры вправо с клавиатуры (Правый стик вправо)",
     "L"},
};

std::mutex key_mutex;
std::array<std::string, KeyActionCount> key_bindings = [] {
    std::array<std::string, KeyActionCount> arr{};
    for (int i = 0; i < KeyActionCount; ++i) {
        arr[i] = kKeyDefs[i].default_val;
    }
    return arr;
}();

const char* Path() {
    const char* env = std::getenv("BB_CONFIG");
    return env && env[0] ? env : "bbport.ini";
}

float Clamp(float v, float lo, float hi) {
    return std::clamp(v, lo, hi);
}

void Set(Values& v, const std::string& key, const std::string& value) {
    const float f = float(std::atof(value.c_str()));
    const int i = std::atoi(value.c_str());
    if (key == "menu_language") {
        if (value == "en") v.menu_language = MenuLanguage::English;
        else if (value == "ru") v.menu_language = MenuLanguage::Russian;
        else std::printf("Settings: unknown menu_language '%s' (expected en or ru)\n", value.c_str());
        v.menu_language_chosen = value == "en" || value == "ru";
    } else if (key == "upscaler") {
        for (int u = 0; u < UpscalerCount; ++u) {
            if (value == UpscalerName(u)) {
                v.upscaler = u;
            }
        }
    } else if (key == "aa_mode") {
        for (int a = 0; a < AaCount; ++a) {
            if (value == AaModeName(a)) {
                v.aa_mode = a;
                v.aa_mode_chosen = true;
            }
        }
    } else if (key == "preset") {
        v.preset = std::clamp(i, 0, PresetCount - 1);
    } else if (key == "sharpen") {
        v.sharpen = i != 0;
    } else if (key == "sharpness") {
        v.sharpness = Clamp(f, 0.0f, 2.0f);
    } else if (key == "jitter") {
        v.jitter = i != 0;
    } else if (key == "reactive") {
        v.reactive = i != 0;
    } else if (key == "object_motion") {
        v.object_motion = i != 0;
    } else if (key == "reactive_scale") {
        v.reactive_scale = Clamp(f, 0.0f, 16.0f);
    } else if (key == "reactive_threshold") {
        v.reactive_threshold = Clamp(f, 0.0f, 1.0f);
    } else if (key == "reactive_max") {
        v.reactive_max = Clamp(f, 0.0f, 1.0f);
    } else if (key == "debug_view") {
        v.debug_view = std::clamp(i, 0, DebugViewCount - 1);
    } else if (key == "show_fps") {
        v.show_fps = i != 0;
    } else if (key == "menu_pos") {
        float x = -1.0f, y = -1.0f;
        if (std::sscanf(value.c_str(), "%f,%f", &x, &y) == 2 && x >= 0.0f && x <= 1.0f && y >= 0.0f &&
            y <= 1.0f) {
            v.menu_x = x;
            v.menu_y = y;
        }
    } else if (key == "fsr4_auto_exposure") {
        v.fsr4_auto_exposure = i != 0;
    } else if (key == "fsr4_invert_jitter") {
        v.fsr4_invert_jitter = i != 0;
    } else if (key == "model_lod") {
        v.model_lod = std::clamp(i, -2, 2);
    } else if (key == "frame_generation") {
        v.frame_generation = i != 0;
    } else if (key == "live_resolution") {
        v.live_resolution = value == "auto" ? -1 : std::clamp(i, 0, 1);
    } else if (key == "output_res") {
        int w = 0, h = 0;
        if (std::sscanf(value.c_str(), "%dx%d", &w, &h) == 2 && w >= 320 && h >= 200 && w <= 16384 &&
            h <= 16384) {
            v.output_res = PackSize({w, h});
        }
    } else if (key == "aspect_ratio") {
        v.aspect = AspectAuto;
        for (int a = 0; a < AspectCount; ++a) {
            if (value == AspectKey(a)) v.aspect = a;
        }
    } else if (key == "mouse_look") {
        v.mouse_look = i != 0;
    } else if (key == "mouse_invert_y") {
        v.mouse_invert_y = i != 0;
    } else if (key == "mouse_sensitivity") {
        v.mouse_sensitivity = Clamp(f, 0.05f, 20.0f);
    } else if (key == "dlss_preset") {
        v.dlss_preset = (i == 0 || (i >= 10 && i <= 13)) ? i : 11;
    } else if (key == "anisotropic") {
        v.anisotropic = (i >= 16) ? 16 : (i >= 8) ? 8 : (i >= 4) ? 4 : 0;
    } else if (key == "fps_limit") {
        v.fps_limit = (i <= 0) ? 0 : std::clamp(i, 30, 5000);
    } else if (key == "ui_upscale") {
        v.ui_upscale = (value != "0" && value != "false" && value != "off");
    } else if (key == "ssao_quality") {
        v.ssao_quality = (value != "0" && value != "false" && value != "off");
    } else if (key == "contact_shadows") {
        v.contact_shadows = (value != "0" && value != "false" && value != "off");
    } else if (key == "ssr_quality") {
        v.ssr_quality = (value != "0" && value != "false" && value != "off");
    } else if (key == "true_black_hdr") {
        v.true_black_hdr = (value != "0" && value != "false" && value != "off");
    } else {
        for (int k = 0; k < KeyActionCount; ++k) {
            if (key == kKeyDefs[k].ini_key) {
                std::string trimmed = value;
                const auto first = trimmed.find_first_not_of(" \t");
                if (first != std::string::npos) {
                    trimmed = trimmed.substr(first, trimmed.find_last_not_of(" \t") - first + 1);
                } else {
                    trimmed.clear();
                }
                if (trimmed == "E, Mouse X2" || trimmed == "E") trimmed = "E, Mouse 5";
                else if (trimmed == "Left Shift+Mouse Left, Mouse X1" || trimmed == "Left Shift+Mouse Left")
                    trimmed = "Left Shift+Mouse Left, Mouse 4";
                else if (trimmed == "Tab, R") trimmed = "R, Tab";
                else if (trimmed == "F, Return") trimmed = "F, Enter";
                else if (trimmed == "3, Up, Wheel Up" || trimmed == "Wheel Up, 3, Up") trimmed = "Wheel Up, 3";
                else if (trimmed == "4, Down, Wheel Down" || trimmed == "Wheel Down, 4, Down") trimmed = "Wheel Down, 4";
                else if (trimmed == "1, Left") trimmed = "1";
                else if (trimmed == "2, Right") trimmed = "2";
                if (trimmed == "None" || trimmed == "none" || trimmed == "Unbound" || trimmed == "Delete") {
                    trimmed.clear();
                }
                {
                    std::scoped_lock lock{key_mutex};
                    key_bindings[k] = trimmed;
                }
                return;
            }
        }
        for (int e = 0; e < EffectCount; ++e) {
            if (key == Effects[e].key) {
                v.effects[e] = i != 0;
            }
        }
    }
}

} // namespace

const KeyActionDef& GetKeyActionDef(int action) {
    return kKeyDefs[std::clamp(action, 0, KeyActionCount - 1)];
}

std::string GetKeyBinding(int action) {
    std::scoped_lock lock{key_mutex};
    return key_bindings[std::clamp(action, 0, KeyActionCount - 1)];
}

void SetKeyBinding(int action, const std::string& value) {
    const int idx = std::clamp(action, 0, KeyActionCount - 1);
    std::scoped_lock lock{key_mutex};
    if (value == "None" || value == "none" || value == "Unbound" || value == "Delete") {
        key_bindings[idx].clear();
    } else {
        key_bindings[idx] = value;
    }
}

std::string FormatKeyBinding(const std::string& raw, bool russian) {
    auto trim = [](std::string s) {
        const auto a = s.find_first_not_of(" \t");
        if (a == std::string::npos) return std::string{};
        return s.substr(a, s.find_last_not_of(" \t") - a + 1);
    };
    auto pretty_token = [](const std::string& tok) -> std::string {
        if (tok == "Return") return "Enter";
        if (tok == "Mouse X1") return "Mouse 4";
        if (tok == "Mouse X2") return "Mouse 5";
        return tok;
    };
    std::string out;
    std::size_t pos = 0;
    while (pos < raw.size()) {
        const std::size_t comma = raw.find(',', pos);
        std::string alt = trim(raw.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos));
        pos = comma == std::string::npos ? raw.size() : comma + 1;
        if (alt.empty() || alt == "None" || alt == "none" || alt == "Unbound") continue;
        std::string chord_out;
        std::size_t p2 = 0;
        while (p2 < alt.size()) {
            const std::size_t plus = alt.find('+', p2);
            // Handle "Keypad +" where '+' is the last character
            std::string tok;
            if (plus != std::string::npos && plus + 1 < alt.size()) {
                tok = trim(alt.substr(p2, plus - p2));
                p2 = plus + 1;
            } else {
                tok = trim(alt.substr(p2));
                p2 = alt.size();
            }
            if (tok.empty()) continue;
            if (!chord_out.empty()) chord_out += " + ";
            chord_out += pretty_token(tok);
        }
        if (chord_out.empty()) continue;
        if (!out.empty()) out += " / ";
        out += chord_out;
    }
    if (out.empty()) {
        return russian ? "Нет" : "None";
    }
    return out;
}

void ResetKeyBindingsToDefault() {
    {
        std::scoped_lock lock{key_mutex};
        for (int i = 0; i < KeyActionCount; ++i) {
            key_bindings[i] = kKeyDefs[i].default_val;
        }
    }
    auto& v = Get();
    v.mouse_look = true;
    v.mouse_invert_y = false;
    v.mouse_sensitivity = 1.0f;
}

namespace {
struct Group {
    const char* key;
    double ratio;
    std::array<OutputSize, 10> sizes;
    int count;
};
constexpr Group Groups[AspectCount] = {
    {"auto", 0.0, {}, 0},
    {"4:3", 4.0 / 3.0,
     {{{640, 480}, {800, 600}, {1024, 768}, {1280, 960}, {1440, 1080}, {1600, 1200}, {2048, 1536}, {2880, 2160}}}, 8},
    {"16:10", 16.0 / 10.0,
     {{{640, 400}, {960, 600}, {1280, 800}, {1440, 900}, {1680, 1050}, {1920, 1200}, {2560, 1600}, {3840, 2400}}}, 8},
    {"16:9", 16.0 / 9.0,
     {{{640, 360}, {854, 480}, {960, 540}, {1280, 720}, {1600, 900}, {1920, 1080}, {2560, 1440}, {3200, 1800},
       {3840, 2160}}}, 9},
    // 2560x1080 and 5120x2160 are 64:27, 3440x1440 43:18, 3840x1600 12:5: each output's own
    // ratio is the camera's.
    {"21:9", 64.0 / 27.0,
     {{{860, 360}, {1280, 540}, {1720, 720}, {2560, 1080}, {3440, 1440}, {3840, 1600}, {5120, 2160}}}, 7},
    {"32:9", 32.0 / 9.0, {{{1280, 360}, {1920, 540}, {2560, 720}, {3840, 1080}, {5120, 1440}, {7680, 2160}}}, 6},
};
std::atomic<std::uint32_t> monitor{0}; // PackSize, 0 until known
} // namespace

int OutputList::IndexOf(OutputSize size) const {
    for (int i = 0; i < count; ++i) {
        if (sizes[i] == size) return i;
    }
    return -1;
}

const char* AspectKey(int aspect) {
    return Groups[std::clamp(aspect, 0, AspectCount - 1)].key;
}

OutputSize Monitor() {
    return UnpackSize(monitor.load(std::memory_order_relaxed));
}

int ResolvedAspect(int aspect) {
    if (aspect > AspectAuto && aspect < AspectCount) return aspect;
    const OutputSize m = Monitor();
    if (m.width <= 0 || m.height <= 0) return Aspect16x9;
    const double ratio = double(m.width) / m.height;
    int best = Aspect16x9;
    for (int a = AspectAuto + 1; a < AspectCount; ++a) {
        if (std::abs(std::log(ratio / Groups[a].ratio)) < std::abs(std::log(ratio / Groups[best].ratio))) {
            best = a;
        }
    }
    return best;
}

OutputList Outputs(int aspect) {
    const Group& g = Groups[ResolvedAspect(aspect)];
    OutputList list;
    for (int i = 0; i < g.count; ++i) list.sizes[list.count++] = g.sizes[i];
    // Auto: the monitor's own size too (another ratio than its group's sizes, 1366x768, ...).
    const OutputSize m = Monitor();
    if (aspect == AspectAuto && m.width >= 640 && m.height >= 360 && m.width <= 7680 &&
        m.height <= 4320 && list.IndexOf(m) < 0) {
        int at = 0;
        while (at < list.count && (list.sizes[at].height < m.height ||
                                   (list.sizes[at].height == m.height && list.sizes[at].width < m.width))) {
            ++at;
        }
        for (int i = list.count; i > at; --i) list.sizes[i] = list.sizes[i - 1];
        list.sizes[at] = m;
        ++list.count;
    }
    return list;
}

OutputSize NearestOutput(int aspect, OutputSize size) {
    const OutputList list = Outputs(aspect);
    if (list.IndexOf(size) >= 0) return size;
    OutputSize best = list.sizes[0];
    for (const OutputSize& s : list) {
        if (std::abs(s.height - size.height) < std::abs(best.height - size.height)) best = s;
    }
    return best;
}

OutputSize CurrentOutput() {
    const auto& v = Get();
    return NearestOutput(v.aspect.load(std::memory_order_relaxed),
                         UnpackSize(v.output_res.load(std::memory_order_relaxed)));
}

std::string SizeText(OutputSize size) {
    return std::to_string(size.width) + "x" + std::to_string(size.height);
}

void SetMonitor(int width, int height) {
    const OutputSize m{width, height};
    if (width <= 0 || height <= 0 || m == Monitor()) return;
    monitor.store(PackSize(m), std::memory_order_relaxed);
    auto& v = Get();
    // Auto follows the monitor: its output is kept (and saved: run.sh's startup patch reads it).
    const OutputSize fitted = CurrentOutput();
    if (v.aspect == AspectAuto && PackSize(fitted) != v.output_res.load()) {
        std::printf("Output: %s for the monitor (%dx%d, aspect ratio auto)\n", SizeText(fitted).c_str(),
                    width, height);
        v.output_res = PackSize(fitted);
        Save();
    }
}

Values& Get() {
    static Values values;
    return values;
}

const char* MenuText(const char* english, const char* russian) {
    return Get().menu_language == MenuLanguage::Russian ? russian : english;
}

void Load() {
    auto& v = Get();
    for (int e = 0; e < EffectCount; ++e) {
        v.effects[e] = Effects[e].default_on;
    }
    // Default menu language follows BB_MENU_LANGUAGE or BB_LANGUAGE (8 = Russian; English by default).
    {
        const char* menu_lang = std::getenv("BB_MENU_LANGUAGE");
        const char* game_lang = std::getenv("BB_LANGUAGE");
        const bool is_ru = (menu_lang && (menu_lang[0] == 'r' || menu_lang[0] == 'R') &&
                            (menu_lang[1] == 'u' || menu_lang[1] == 'U')) ||
                           ((!menu_lang || !*menu_lang) && game_lang && std::atoi(game_lang) == 8);
        v.menu_language = is_ru ? MenuLanguage::Russian : MenuLanguage::English;
    }
    if (FILE* file = std::fopen(Path(), "r")) {
        char line[256];
        while (std::fgets(line, sizeof(line), file)) {
            std::string text{line};
            text.erase(text.find_last_not_of(" \t\r\n") + 1);
            const auto eq = text.find('=');
            if (text.empty() || text[0] == '#' || eq == std::string::npos) {
                continue;
            }
            Set(v, text.substr(0, eq), text.substr(eq + 1));
        }
        std::fclose(file);
        std::printf("Settings: %s\n", Path());
    }
    // Environment overrides (scripts, A/B tests).
    if (const char* env = std::getenv("BB_UPSCALER")) {
        v.upscaler = UpscalerOff;
        for (int u = 0; u < UpscalerCount; ++u) {
            if (std::strcmp(env, UpscalerName(u)) == 0) v.upscaler = u;
        }
    }
    const std::pair<const char*, const char*> env_keys[] = {
        {"BB_FSR_SHARPNESS", "sharpness"},        {"BB_JITTER", "jitter"},
        {"BB_REACTIVE", "reactive"},              {"BB_REACTIVE_SCALE", "reactive_scale"},
        {"BB_REACTIVE_THRESHOLD", "reactive_threshold"}, {"BB_REACTIVE_MAX", "reactive_max"},
        {"BB_UPSCALE_PRESET", "preset"},            {"BB_OBJECT_MOTION", "object_motion"},
        {"BB_FRAME_GEN", "frame_generation"},       {"BB_UI_UPSCALE", "ui_upscale"},
        {"BB_SSAO_QUALITY", "ssao_quality"},        {"BB_CONTACT_SHADOWS", "contact_shadows"},
        {"BB_SSR_QUALITY", "ssr_quality"},          {"BB_TRUE_BLACK_HDR", "true_black_hdr"},
    };
    // Frame generation is hidden in the menu until its artifacts are fixed: only BB_FRAME_GEN turns
    // it on, a saved setting does not.
    v.frame_generation = false;
    for (const auto& [env, key] : env_keys) {
        if (const char* value = std::getenv(env)) {
            Set(v, key, value);
        }
    }
    if (const char* env = std::getenv("BB_ANISO")) {
        Set(v, "anisotropic", env);
    }
    if (const char* env = std::getenv("BB_FPS_LIMIT")) {
        if (std::atoi(env) > 0) {
            Set(v, "fps_limit", env);
        }
    }
    if (!v.aa_mode_chosen.load()) {
        if (v.upscaler.load() == UpscalerTaa) {
            v.aa_mode = AaTaa;
            v.upscaler = UpscalerOff;
        } else {
            v.aa_mode = v.effects[4].load() ? AaFxaa : AaOff;
        }
    } else {
        if (v.upscaler.load() == UpscalerTaa) {
            v.upscaler = UpscalerOff;
        }
        v.effects[4] = (v.aa_mode.load() == AaFxaa);
    }
    BbFrameRate::SetLimit(v.fps_limit.load());
    v.startup_preset = v.preset;
    v.startup_upscaler = v.upscaler;
    v.startup_object_motion = v.object_motion;
    for (int e = 0; e < EffectCount; ++e) {
        v.startup_effects[e] = v.effects[e];
    }
    v.startup_model_lod = v.model_lod;
    if (FixedRenderSession()) {
        int w = 0, h = 0;
        const char* output = std::getenv("BB_OUTPUT_RES");
        if (output && std::sscanf(output, "%dx%d", &w, &h) == 2) v.startup_output = {w, h};
    }
    v.startup_live_resolution = v.live_resolution;
}

void ConfigureUpscalerSupport(bool fsr4, bool fsr411) {
    auto& v = Get();
    v.fsr4_supported = fsr4;
    v.fsr411_supported = fsr4 && fsr411;
    const int requested = v.upscaler;
    if ((requested == UpscalerFsr4 && !v.fsr4_supported) ||
        (requested == UpscalerFsr411 && !v.fsr411_supported)) {
        v.fsr4_problem = "GPU does not support the selected FSR 4 shaders; using FSR 3.1";
        std::printf("Upscaler: %s unsupported on this GPU; falling back to FSR 3.1 before the first frame\n",
                    UpscalerName(requested));
        v.upscaler = UpscalerFsr3;
    }
}

void ConfigureDlssSupport(bool available, const char* problem) {
    auto& v = Get();
    v.dlss_supported = available;
    static std::string kept;
    kept = problem ? problem : "";
    v.dlss_problem = available || kept.empty() ? nullptr : kept.c_str();
    if (v.upscaler == UpscalerDlss && !available) {
        std::printf("Upscaler: DLSS unavailable (%s); falling back to FSR 3.1\n",
                    kept.empty() ? "the DLSS bridge or NVIDIA's DLSS library is missing" : kept.c_str());
        v.upscaler = UpscalerFsr3;
    }
}

bool FixedRenderSession() {
    const char* size = std::getenv("BB_RENDER_RES");
    return size && size[0];
}

int EffectiveUpscaler() {
    const auto& v = Get();
    const int up = v.upscaler.load(std::memory_order_relaxed);
    if (up != UpscalerOff && up != UpscalerTaa) {
        return up;
    }
    const int aa = v.aa_mode.load(std::memory_order_relaxed);
    if (aa == AaTaa) {
        return UpscalerTaa;
    }
    if (aa == AaDlaa) {
        return v.dlss_supported.load(std::memory_order_relaxed) ? UpscalerDlss : UpscalerTaa;
    }
    return UpscalerOff;
}

int RenderPreset() {
    const auto& v = Get();
    const int up = v.upscaler.load(std::memory_order_relaxed);
    if (up == UpscalerOff || up == UpscalerTaa) {
        return NativeAA;
    }
    return v.preset.load(std::memory_order_relaxed);
}

bool ResolutionNeedsRestart() {
    const auto& v = Get();
    return FixedRenderSession() && CurrentOutput() != v.startup_output;
}

void Save() {
    const auto& v = Get();
    // The keys the menu writes, in this order; the rest of the file stays as it is: the
    // launcher's controls (key.* / pad.*) and its other keys, comments.
    std::vector<std::pair<std::string, std::string>> keys;
    const auto put = [&](const char* key, std::string value) { keys.emplace_back(key, std::move(value)); };
    const auto fixed = [](float value, int digits) {
        char text[32];
        std::snprintf(text, sizeof(text), "%.*f", digits, value);
        return std::string(text);
    };
    const auto flag = [](bool value) { return std::string(value ? "1" : "0"); };
    if (v.menu_language_chosen) {
        put("menu_language", v.menu_language == MenuLanguage::Russian ? "ru" : "en");
    }
    put("aa_mode", AaModeName(v.aa_mode));
    put("upscaler", UpscalerName(v.upscaler == UpscalerTaa ? UpscalerOff : v.upscaler.load()));
    put("preset", std::to_string(v.preset.load()));
    put("sharpen", flag(v.sharpen));
    put("sharpness", fixed(v.sharpness, 2));
    put("jitter", flag(v.jitter));
    put("reactive", flag(v.reactive));
    put("object_motion", flag(v.object_motion));
    put("reactive_scale", fixed(v.reactive_scale, 2));
    put("reactive_threshold", fixed(v.reactive_threshold, 2));
    put("reactive_max", fixed(v.reactive_max, 2));
    put("debug_view", std::to_string(v.debug_view.load()));
    put("show_fps", flag(v.show_fps));
    put("fsr4_auto_exposure", flag(v.fsr4_auto_exposure));
    put("fsr4_invert_jitter", flag(v.fsr4_invert_jitter));
    // Read by patches.py at start.
    for (int e = 0; e < EffectCount; ++e) {
        put(Effects[e].key, flag(v.effects[e]));
    }
    put("model_lod", std::to_string(v.model_lod.load()));
    put("aspect_ratio", AspectKey(v.aspect));
    put("output_res", SizeText(CurrentOutput()));
    // Read by run.sh at start.
    put("live_resolution", v.live_resolution < 0 ? "auto" : flag(v.live_resolution != 0));
    put("frame_generation", flag(v.frame_generation));
    if (v.menu_x >= 0.0f && v.menu_y >= 0.0f) {
        put("menu_pos", fixed(v.menu_x, 4) + "," + fixed(v.menu_y, 4));
    }
    put("mouse_look", flag(v.mouse_look));
    put("mouse_invert_y", flag(v.mouse_invert_y));
    put("mouse_sensitivity", fixed(v.mouse_sensitivity, 2));
    put("dlss_preset", std::to_string(v.dlss_preset.load()));
    put("anisotropic", std::to_string(v.anisotropic.load()));
    put("fps_limit", std::to_string(v.fps_limit.load()));
    put("ui_upscale", flag(v.ui_upscale.load()));
    put("ssao_quality", flag(v.ssao_quality.load()));
    put("contact_shadows", flag(v.contact_shadows.load()));
    put("ssr_quality", flag(v.ssr_quality.load()));
    put("true_black_hdr", flag(v.true_black_hdr.load()));
    for (int k = 0; k < KeyActionCount; ++k) {
        put(kKeyDefs[k].ini_key, GetKeyBinding(k));
    }

    std::string out;
    bool had_lines = false;
    std::vector<bool> written(keys.size());
    if (FILE* file = std::fopen(Path(), "r")) {
        char line[512];
        while (std::fgets(line, sizeof(line), file)) {
            had_lines = true;
            std::string text{line};
            text.erase(text.find_last_not_of("\r\n") + 1);
            const auto eq = text.find('=');
            if (!text.empty() && text[0] != '#' && eq != std::string::npos) {
                std::string key = text.substr(0, eq);
                key.erase(key.find_last_not_of(" \t") + 1);
                key.erase(0, key.find_first_not_of(" \t"));
                const auto it = std::find_if(keys.begin(), keys.end(),
                                             [&](const auto& k) { return k.first == key; });
                if (it != keys.end()) {
                    out += it->first + "=" + it->second + "\n";
                    written[size_t(it - keys.begin())] = true;
                    continue;
                }
            }
            out += text + "\n";
        }
        std::fclose(file);
    }
    if (!had_lines) {
        out = "# bbport settings (in-game menu: Insert / L3+R3)\n";
    }
    for (size_t k = 0; k < keys.size(); ++k) {
        if (!written[k]) {
            out += keys[k].first + "=" + keys[k].second + "\n";
        }
    }
    // Replaced whole: a crash while writing leaves the old file.
    const std::string temporary = std::string(Path()) + ".tmp";
    FILE* file = std::fopen(temporary.c_str(), "w");
    bool ok = file && std::fwrite(out.data(), 1, out.size(), file) == out.size();
    ok = file && std::fclose(file) == 0 && ok;
    if (!ok || std::rename(temporary.c_str(), Path()) != 0) {
        std::printf("Settings: cannot write %s\n", Path());
    } else if (runtime_pad_reload_bindings) {
        runtime_pad_reload_bindings();
    }
}

float PresetScale(int preset) {
    static constexpr float scales[PresetCount] = {1.0f, 1.5f, 1.7f, 2.0f, 3.0f};
    return scales[std::clamp(preset, 0, PresetCount - 1)];
}

const char* PresetName(int preset) {
    static constexpr const char* names[PresetCount] = {"Native AA", "Quality", "Balanced",
                                                       "Performance", "Ultra Performance"};
    return names[std::clamp(preset, 0, PresetCount - 1)];
}

const char* UpscalerName(int upscaler) {
    static constexpr const char* names[UpscalerCount] = {"off", "fsr3", "fsr4", "fsr411", "taa", "dlss"};
    return names[std::clamp(upscaler, 0, UpscalerCount - 1)];
}

const char* AaModeName(int aa) {
    static constexpr const char* names[AaCount] = {"off", "fxaa", "taa", "dlaa"};
    return names[std::clamp(aa, 0, AaCount - 1)];
}

} // namespace BbSettings
