// SPDX-License-Identifier: GPL-2.0-or-later
#include "bbport_overlay.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>
#include "bbport_guest_hooks.h"
#include "bbport_frame_rate.h"
#include "bbport_sections.h"
#include "bbport_settings.h"
#include "bbport_toggles.h"
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "vulkan/instance.h"
#include "render/direct_shaders.h"
#include "render/scheduler.h"
#include "../../our_cpp_100000_source/include/gx_direct_vulkan.h"

// DejaVu Sans (Cyrillic), embedded (third_party/fonts, Bitstream Vera license).
#ifdef _WIN32
#define BB_FONT_SECTION ".section .rdata,\"dr\"\n"
#define BB_FONT_HIDDEN(name) ""
#define BB_FONT_END ".text\n"
#else
#define BB_FONT_SECTION ".section .rodata\n"
#define BB_FONT_HIDDEN(name) ".hidden " name "\n"
#define BB_FONT_END ".previous\n"
#endif
asm(BB_FONT_SECTION
    ".balign 16\n"
    BB_FONT_HIDDEN("bb_font_ttf")
    ".global bb_font_ttf\n"
    "bb_font_ttf:\n"
    ".incbin \"" BB_FONT_PATH "\"\n"
    BB_FONT_HIDDEN("bb_font_ttf_end")
    ".global bb_font_ttf_end\n"
    "bb_font_ttf_end:\n"
    BB_FONT_END);
extern "C" const unsigned char bb_font_ttf[];
extern "C" const unsigned char bb_font_ttf_end[];

extern "C" void runtime_restart(void); // bb-probe (probe.c)
extern "C" __attribute__((weak)) void runtime_toypad_lights(uint32_t out[3]); // runtime_toypad.c

namespace BbOverlay {

namespace {

std::mutex imgui_mutex; // the ImGui context: window thread (input) and present thread
bool initialized = false;
std::atomic<bool> menu_open{false};
std::atomic<bool> reset_hud_avg{false};
bool l3_down = false, r3_down = false;
bool dirty = false; // settings changed while open: saved on close
int rebinding_action = -1; // >= 0 while waiting for a key/mouse press to rebind KeyAction
bool rebind_wait_initial_release = false;
std::vector<std::string> rebind_tokens;
float base_scale = 1.0f;
// The game's text dialog (ImeDialog, the character name), typed on the keyboard: drawn while it
// is open. In fullscreen the window title that showed it is not visible (issues #17, #19).
std::mutex prompt_mutex;
std::atomic<bool> prompt_active{false};
/// A module's line over the game (bbgpu_overlay_banner: BBSC's "joining the host" during its
/// loads, where the game itself shows a black screen).
std::mutex banner_mutex;
std::string banner_text;
std::atomic<bool> banner_active{false};
std::string prompt_title, prompt_text;

// Present rate for the FPS counter.
std::chrono::steady_clock::time_point last_present{};
float frame_ms_avg = 0.0f;
double present_ms_sum_accum = 0.0;
double present_ms_sq_sum_accum = 0.0;
std::uint64_t present_ms_count_accum = 0;

float PixelDensity(SDL_WindowID id);

void SetOpen(bool value) {
    if (menu_open.exchange(value) == value) {
        return;
    }
    // The system cursor shows over the menu (window.cpp); ImGui learns where it is now, not at
    // the next motion: mouse motion is not passed on while the menu is closed.
    if (value) {
        if (SDL_Window* window = SDL_GetMouseFocus()) {
            float x = 0.0f, y = 0.0f;
            SDL_GetMouseState(&x, &y);
            const float density = PixelDensity(SDL_GetWindowID(window));
            ImGui::GetIO().AddMousePosEvent(x * density, y * density);
        }
    }
    if (!value) {
        rebinding_action = -1;
    }
    if (!value && dirty) {
        dirty = false;
        BbSettings::Save();
    }
}

ImGuiKey KeyFromSdl(SDL_Keycode key) {
    switch (key) {
    case SDLK_TAB: return ImGuiKey_Tab;
    case SDLK_LEFT: return ImGuiKey_LeftArrow;
    case SDLK_RIGHT: return ImGuiKey_RightArrow;
    case SDLK_UP: return ImGuiKey_UpArrow;
    case SDLK_DOWN: return ImGuiKey_DownArrow;
    case SDLK_PAGEUP: return ImGuiKey_PageUp;
    case SDLK_PAGEDOWN: return ImGuiKey_PageDown;
    case SDLK_HOME: return ImGuiKey_Home;
    case SDLK_END: return ImGuiKey_End;
    case SDLK_DELETE: return ImGuiKey_Delete;
    case SDLK_BACKSPACE: return ImGuiKey_Backspace;
    case SDLK_SPACE: return ImGuiKey_Space;
    case SDLK_RETURN: return ImGuiKey_Enter;
    case SDLK_KP_ENTER: return ImGuiKey_KeypadEnter;
    case SDLK_ESCAPE: return ImGuiKey_Escape;
    case SDLK_LCTRL: return ImGuiKey_LeftCtrl;
    case SDLK_RCTRL: return ImGuiKey_RightCtrl;
    case SDLK_LSHIFT: return ImGuiKey_LeftShift;
    case SDLK_RSHIFT: return ImGuiKey_RightShift;
    case SDLK_LALT: return ImGuiKey_LeftAlt;
    case SDLK_RALT: return ImGuiKey_RightAlt;
    default: return ImGuiKey_None;
    }
}

ImGuiKey KeyFromGamepad(u8 button) {
    switch (button) {
    case SDL_GAMEPAD_BUTTON_SOUTH: return ImGuiKey_GamepadFaceDown;
    case SDL_GAMEPAD_BUTTON_EAST: return ImGuiKey_GamepadFaceRight;
    case SDL_GAMEPAD_BUTTON_WEST: return ImGuiKey_GamepadFaceLeft;
    case SDL_GAMEPAD_BUTTON_NORTH: return ImGuiKey_GamepadFaceUp;
    case SDL_GAMEPAD_BUTTON_DPAD_UP: return ImGuiKey_GamepadDpadUp;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return ImGuiKey_GamepadDpadDown;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return ImGuiKey_GamepadDpadLeft;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return ImGuiKey_GamepadDpadRight;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return ImGuiKey_GamepadL1;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return ImGuiKey_GamepadR1;
    case SDL_GAMEPAD_BUTTON_START: return ImGuiKey_GamepadStart;
    case SDL_GAMEPAD_BUTTON_BACK: return ImGuiKey_GamepadBack;
    default: return ImGuiKey_None;
    }
}

float PixelDensity(SDL_WindowID id) {
    SDL_Window* window = SDL_GetWindowFromID(id);
    const float density = window ? SDL_GetWindowPixelDensity(window) : 1.0f;
    return density > 0.0f ? density : 1.0f;
}

// Marks the settings dirty when a widget changed them.
template <typename T>
void Store(std::atomic<T>& target, T value, bool changed) {
    if (changed) {
        target = value;
        dirty = true;
    }
}

void Checkbox(const char* label, std::atomic<bool>& value) {
    bool v = value;
    Store(value, v, ImGui::Checkbox(label, &v));
}

void Slider(const char* label, std::atomic<float>& value, float lo, float hi) {
    float v = value;
    Store(value, v, ImGui::SliderFloat(label, &v, lo, hi, "%.2f"));
}

void Hint(const char* text) {
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

// The USB toy pad: the figures on the emulated pad, put on and lifted off from
// the menu with the gamepad. The runtime names the figures file (BB_TOYPAD_FILE) once the game
// opens the pad and reads it again when it changes; this section edits that file.
struct ToyPadFigure {
    int zone;
    unsigned id;
};
struct ToyPadName {
    unsigned id;
    std::string name;
};

std::vector<ToyPadFigure> ReadToyPad(const char* path) {
    std::vector<ToyPadFigure> figures;
    FILE* file = std::fopen(path, "r");
    char line[256];
    while (file && std::fgets(line, sizeof(line), file)) {
        if (char* hash = std::strchr(line, '#')) {
            *hash = 0;
        }
        int zone;
        unsigned id;
        if (std::sscanf(line, "%d %u", &zone, &id) == 2 && zone >= 1 && zone <= 3 && id) {
            figures.push_back({zone, id});
        }
    }
    if (file) {
        std::fclose(file);
    }
    return figures;
}

// Figure names: "<id> <name>" lines in figure_ids.txt next to the figures file (BB_TOYPAD_NAMES
// names another one); without it the ids are shown.
const std::vector<ToyPadName>& ToyPadNames(const char* figures_path) {
    static std::vector<ToyPadName> names;
    static bool loaded;
    if (!loaded) {
        loaded = true;
        std::string path;
        if (const char* env = std::getenv("BB_TOYPAD_NAMES"); env && *env) {
            path = env;
        } else {
            path = figures_path;
            const size_t slash = path.rfind('/');
            path = (slash == std::string::npos ? std::string{} : path.substr(0, slash + 1)) + "figure_ids.txt";
        }
        FILE* file = std::fopen(path.c_str(), "r");
        char line[256];
        while (file && std::fgets(line, sizeof(line), file)) {
            unsigned id;
            int used = 0;
            if (std::sscanf(line, "%u %n", &id, &used) == 1 && id) {
                std::string name = line + used;
                while (!name.empty() && (name.back() == '\n' || name.back() == '\r' || name.back() == ' ')) {
                    name.pop_back();
                }
                names.push_back({id, name});
            }
        }
        if (file) {
            std::fclose(file);
        }
    }
    return names;
}

std::string ToyPadLabel(const std::vector<ToyPadName>& names, unsigned id) {
    for (const auto& n : names) {
        if (n.id == id) {
            return n.name + " (" + std::to_string(id) + ")";
        }
    }
    return "#" + std::to_string(id);
}

void WriteToyPad(const char* path, const std::vector<ToyPadFigure>& figures,
                 const std::vector<ToyPadName>& names) {
    const std::string temp = std::string{path} + ".tmp";
    FILE* file = std::fopen(temp.c_str(), "w");
    if (!file) {
        return;
    }
    std::fputs("# Figures on the virtual Toy Pad: \"<zone> <figure id>\", one per line.\n"
               "# Zone 1: centre (1 figure), 2: left (3), 3: right (3). Also edited from the menu "
               "(Insert / L3+R3).\n", file);
    for (const auto& f : figures) {
        std::fprintf(file, "%d %u  # %s\n", f.zone, f.id, ToyPadLabel(names, f.id).c_str());
    }
    std::fclose(file);
    std::rename(temp.c_str(), path);
}

void ToyPadSection() {
    const char* path = std::getenv("BB_TOYPAD_FILE");
    if (!path || !*path) {
        return;
    }
    const auto& names = ToyPadNames(path);
    auto figures = ReadToyPad(path);
    bool changed = false;
    u32 lights[3] = {};
    if (runtime_toypad_lights) {
        runtime_toypad_lights(lights);
    }
    static unsigned pick_id = 1; // the figure to put on
    ImGui::SeparatorText(BbSettings::MenuText("Toy Pad", "Toy Pad (фигурки)"));
    // Choosing a figure: characters, then vehicles and gadgets (ids from 1000).
    const std::string current = ToyPadLabel(names, pick_id);
    if (ImGui::BeginCombo(BbSettings::MenuText("Figure", "Фигурка"), current.c_str(),
                          ImGuiComboFlags_HeightLarge)) {
        for (int vehicles = 0; vehicles < 2; ++vehicles) {
            ImGui::SeparatorText(vehicles ? BbSettings::MenuText("Vehicles and gadgets", "Техника")
                                          : BbSettings::MenuText("Characters", "Персонажи"));
            for (const auto& n : names) {
                if ((n.id >= 1000) != bool(vehicles)) {
                    continue;
                }
                const std::string label = ToyPadLabel(names, n.id);
                if (ImGui::Selectable(label.c_str(), n.id == pick_id)) {
                    pick_id = n.id;
                }
                if (n.id == pick_id && ImGui::IsWindowAppearing()) {
                    ImGui::SetScrollHereY();
                }
            }
        }
        ImGui::EndCombo();
    }
    // The zones in the pad's order (left, centre, right), each lit as the game lights it: the
    // zone it wants a figure on glows.
    static const int order[] = {2, 1, 3};
    static const char* zone_en[] = {"Centre", "Left", "Right"};
    static const char* zone_ru[] = {"Центр", "Левая", "Правая"};
    for (const int zone : order) {
        ImGui::PushID(zone);
        const u32 c = lights[zone - 1];
        const ImVec4 colour = c ? ImVec4(((c >> 16) & 255) / 255.0f, ((c >> 8) & 255) / 255.0f,
                                         (c & 255) / 255.0f, 1.0f)
                                : ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
        ImGui::TextColored(colour, "%s", c ? "●" : "○");
        ImGui::SameLine();
        ImGui::TextUnformatted(BbSettings::MenuText(zone_en[zone - 1], zone_ru[zone - 1]));
        int count = 0;
        for (size_t i = 0; i < figures.size(); ++i) {
            if (figures[i].zone != zone) {
                continue;
            }
            ++count;
            ImGui::PushID(int(i));
            ImGui::SameLine();
            const std::string label = ToyPadLabel(names, figures[i].id) + "  ×";
            if (ImGui::SmallButton(label.c_str())) {
                figures.erase(figures.begin() + long(i));
                changed = true;
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
        const bool full = count >= (zone == 1 ? 1 : 3) || figures.size() >= 7;
        if (!full) {
            ImGui::SameLine();
            if (ImGui::SmallButton(BbSettings::MenuText("+ put here", "+ поставить сюда"))) {
                figures.push_back({zone, pick_id});
                changed = true;
            }
        }
        ImGui::PopID();
    }
    Hint(BbSettings::MenuText(
        "Choose a figure, then \"+ put here\" on a zone; press a figure to lift it. The dot is the "
        "zone's light: when the game asks for a figure it lights the zone to put it on. If the "
        "game misses a figure, lift it and put it on again.",
        "Выберите фигурку и нажмите «+ поставить сюда» у зоны; нажатие на фигурку снимает её. "
        "Кружок — подсветка зоны: когда игра просит фигурку, она подсвечивает зону, куда её "
        "ставить. Если игра не заметила фигурку — снимите и поставьте снова."));
    if (changed) {
        WriteToyPad(path, figures, names);
    }
}

// The settings menu as in 0.3 and 0.4: one window (moved with the mouse, its place kept in
// bbport.ini), sections one under the other, ImGui's own widgets and keyboard/gamepad navigation.
void Menu() {
    auto& s = BbSettings::Get();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    // Where it was moved last (bbport.ini menu_pos, a fraction of the screen), kept on screen.
    ImVec2 pos(viewport->WorkPos.x + 40.0f * base_scale, viewport->WorkPos.y + 40.0f * base_scale);
    if (s.menu_x >= 0.0f && s.menu_y >= 0.0f) {
        const float margin = 80.0f * base_scale;
        pos.x = viewport->WorkPos.x +
                std::clamp(s.menu_x * viewport->WorkSize.x, 0.0f, std::max(viewport->WorkSize.x - margin, 0.0f));
        pos.y = viewport->WorkPos.y +
                std::clamp(s.menu_y * viewport->WorkSize.y, 0.0f, std::max(viewport->WorkSize.y - margin, 0.0f));
    }
    ImGui::SetNextWindowPos(pos, ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(ImVec2(620.0f * base_scale, 0.0f), ImGuiCond_Appearing);
    bool keep_open = true;
    if (!ImGui::Begin(
            BbSettings::MenuText("Bloodborne - Graphics  (Insert / L3+R3)###bbport_settings",
                                 "Bloodborne — настройки  (Insert / L3+R3)###bbport_settings"),
            &keep_open, ImGuiWindowFlags_NoCollapse)) {
        ImGui::End();
        return;
    }
    // Moved: remembered (saved with the settings when the menu closes).
    if (!ImGui::IsWindowAppearing() && viewport->WorkSize.x > 0.0f && viewport->WorkSize.y > 0.0f) {
        const ImVec2 at = ImGui::GetWindowPos();
        const float fx = (at.x - viewport->WorkPos.x) / viewport->WorkSize.x;
        const float fy = (at.y - viewport->WorkPos.y) / viewport->WorkSize.y;
        if (std::abs(at.x - pos.x) >= 1.0f || std::abs(at.y - pos.y) >= 1.0f) {
            s.menu_x = fx;
            s.menu_y = fy;
            dirty = true;
        }
    }
    // Always readable, even when the rest of the menu is in Russian.
    static const char* languages[] = {"English", "Русский"};
    int language = s.menu_language == BbSettings::MenuLanguage::Russian ? 1 : 0;
    if (ImGui::Combo("Language", &language, languages, 2)) {
        s.menu_language =
            language == 1 ? BbSettings::MenuLanguage::Russian : BbSettings::MenuLanguage::English;
        s.menu_language_chosen = true;
        BbSettings::Save();
    }
    ImGui::Text(BbSettings::MenuText("%.0f FPS  (%.1f ms)", "%.0f FPS  (%.1f мс)"),
                frame_ms_avg > 0.0f ? 1000.0f / frame_ms_avg : 0.0f, frame_ms_avg);
    ToyPadSection();

    ImGui::SeparatorText(BbSettings::MenuText("Anti-Aliasing & Upscaler", "Сглаживание и апскейлер"));
    {
        const char* aa_labels[] = {
            BbSettings::MenuText("Off", "Выкл"),
            BbSettings::MenuText("FXAA (Subpixel AA)", "FXAA (Субпиксельное сглаживание)"),
            BbSettings::MenuText("TAA (Temporal AA)", "TAA (Темпоральное сглаживание)"),
            BbSettings::MenuText("DLAA (DLSS Native 100%)", "DLAA (Нативное DLSS 100%)")};
        const int cur_up = s.upscaler.load();
        int aa_mode = cur_up == BbSettings::UpscalerTaa
                          ? 2
                          : (cur_up == BbSettings::UpscalerDlss && s.preset.load() == 0)
                                ? 3
                                : s.effects[4].load() ? 1 : 0;
        if (ImGui::BeginCombo(BbSettings::MenuText("Anti-Aliasing", "Сглаживание"), aa_labels[aa_mode])) {
            for (int i = 0; i < 4; ++i) {
                const bool supported = i == 3 ? s.dlss_supported.load() : true;
                ImGui::BeginDisabled(!supported);
                if (ImGui::Selectable(aa_labels[i], i == aa_mode)) {
                    if (i == 0) {
                        Store(s.effects[4], false, true);
                        if (s.upscaler.load() == BbSettings::UpscalerTaa) {
                            Store(s.upscaler, int(BbSettings::UpscalerOff), true);
                        }
                    } else if (i == 1) {
                        Store(s.effects[4], true, true);
                        if (s.upscaler.load() == BbSettings::UpscalerTaa) {
                            Store(s.upscaler, int(BbSettings::UpscalerOff), true);
                        }
                    } else if (i == 2) {
                        Store(s.upscaler, int(BbSettings::UpscalerTaa), true);
                    } else if (i == 3) {
                        Store(s.upscaler, int(BbSettings::UpscalerDlss), true);
                        Store(s.preset, int(BbSettings::NativeAA), true);
                    }
                    BbGuestHooks::ApplyGameEffects();
                }
                ImGui::EndDisabled();
            }
            ImGui::EndCombo();
        }
    }
    const char* upscalers[] = {
        BbSettings::MenuText("Off (Native 100%)", "Выключен (Native 100%)"),
        "FSR 3.1",
        "FSR 4 (INT8)",
        "FSR 4.1.1",
        BbSettings::MenuText("TAA (native anti-aliasing)", "TAA (нативное сглаживание)"),
        "DLSS (NVIDIA RTX)"};
    static_assert(sizeof(upscalers) / sizeof(upscalers[0]) == BbSettings::UpscalerCount);
    int upscaler = s.upscaler;
    const char* active_upscaler_label =
        upscaler == BbSettings::UpscalerTaa ? upscalers[BbSettings::UpscalerOff] : upscalers[upscaler];
    if (ImGui::BeginCombo(BbSettings::MenuText("Upscaler", "Апскейлер"), active_upscaler_label)) {
        for (int i = 0; i < BbSettings::UpscalerCount; ++i) {
            if (i == BbSettings::UpscalerTaa) continue;
            const bool supported = i == BbSettings::UpscalerFsr4     ? s.fsr4_supported.load()
                                   : i == BbSettings::UpscalerFsr411 ? s.fsr411_supported.load()
                                   : i == BbSettings::UpscalerDlss   ? s.dlss_supported.load()
                                                                     : true;
            ImGui::BeginDisabled(!supported);
            if (ImGui::Selectable(upscalers[i], i == upscaler)) {
                Store(s.upscaler, i, true);
            }
            ImGui::EndDisabled();
            if (!supported) {
                ImGui::SameLine();
                ImGui::TextDisabled("%s", BbSettings::MenuText("— not supported by this GPU",
                                                         "— не поддерживается видеокартой"));
            }
        }
        ImGui::EndCombo();
    }
    if (s.upscaler == BbSettings::UpscalerDlss) {
        Hint(BbSettings::MenuText(
            "NVIDIA DLSS Super Resolution (RTX GPUs; Native AA is DLAA). Needs the DLSS bridge "
            "and NVIDIA's library next to the game; otherwise FSR 3.1 is used.",
            "NVIDIA DLSS Super Resolution (видеокарты RTX; Native AA — это DLAA). Нужны мост DLSS "
            "и библиотека NVIDIA рядом с игрой; без них используется FSR 3.1."));
        const char* dlss_models[] = {
            BbSettings::MenuText("DLSS 4.5 (2nd-Gen Transformer — Preset M)",
                                 "DLSS 4.5 (Трансформер 2-го пок. — Preset M)"),
            BbSettings::MenuText("DLSS 4.5 (2nd-Gen Transformer Ultra — Preset L)",
                                 "DLSS 4.5 (Трансформер 2-го пок. Ultra — Preset L)"),
            BbSettings::MenuText("DLSS 4.0 (Vision Transformer — Preset K)",
                                 "DLSS 4.0 (Vision Transformer — Preset K)"),
            BbSettings::MenuText("DLSS (Driver Default / Auto Preset)",
                                 "DLSS (Авто пресет драйвера)")};
        const int cur_dlss_p = s.dlss_preset.load();
        int dlss_model_idx = cur_dlss_p == 13 ? 0 : cur_dlss_p == 12 ? 1 : cur_dlss_p == 0 ? 3 : 2;
        if (ImGui::Combo(BbSettings::MenuText("DLSS Model", "Модель DLSS"), &dlss_model_idx,
                         dlss_models, 4)) {
            const int new_p = dlss_model_idx == 0 ? 13 : dlss_model_idx == 1 ? 12 : dlss_model_idx == 2 ? 11 : 0;
            Store(s.dlss_preset, new_p, true);
        }
    }
    const auto problem_text = [](const char* text) {
        ImGui::PushTextWrapPos();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "%s", text);
        ImGui::PopTextWrapPos();
    };
    if (const char* problem = s.dlss_problem.load(); problem && s.upscaler == BbSettings::UpscalerDlss) {
        char line[192];
        std::snprintf(line, sizeof(line), "DLSS: %s", problem);
        problem_text(line);
    }
    if (const char* problem = s.fsr4_problem.load()) {
        char line[192];
        std::snprintf(line, sizeof(line),
                      BbSettings::MenuText("FSR 4 unavailable: %s", "FSR 4 недоступен: %s"), problem);
        problem_text(line);
        if (!BbSettings::IsFsr4(s.upscaler)) {
            ImGui::PushTextWrapPos();
            ImGui::TextUnformatted(BbSettings::MenuText(
                "The mode selected above is active. You can select FSR 4 again.",
                "Активен режим, выбранный выше. FSR 4 можно выбрать снова."));
            ImGui::PopTextWrapPos();
        }
    }
    if (BbSettings::IsFsr4(s.upscaler)) {
        if (s.upscaler == BbSettings::UpscalerFsr411) {
            Hint(BbSettings::MenuText(
                "FSR 4.1.1: the model from AMD's 4.1.1 DLL, reproduced in Vulkan (output matches "
                "the DLL). Assets are built from your DLL in the launcher.",
                "FSR 4.1.1: модель из DLL AMD 4.1.1, воспроизведённая в Vulkan (результат совпадает "
                "с DLL). Ассеты собираются из вашей DLL в лаунчере."));
        } else {
            Hint(BbSettings::MenuText(
                "FSR 4 in INT8 mode (v07 model from AMD FidelityFX SDK sources). Higher quality "
                "than FSR 3.1, but the pass is more demanding. Changing the preset rebuilds the "
                "model (a brief pause).",
                "FSR 4 в режиме INT8 (модель v07 из исходников AMD FidelityFX SDK). Качество выше, "
                "чем у FSR 3.1, но проход тяжелее. Смена пресета пересобирает модель (короткая "
                "пауза)."));
        }
        Checkbox(BbSettings::MenuText("FSR 4: auto exposure", "FSR 4: авто-экспозиция"),
                 s.fsr4_auto_exposure);
        Checkbox(BbSettings::MenuText("FSR 4: invert jitter sign", "FSR 4: обратный знак jitter"),
                 s.fsr4_invert_jitter);
        Hint(BbSettings::MenuText(
            "For diagnosing ghosting: the FSR 4 network normalizes color by exposure and uses "
            "it to decide when to discard previous frames. Changes apply immediately, without a "
            "restart.",
            "Проверка при гостинге: сеть FSR 4 нормирует цвет по экспозиции и по ней решает, "
            "когда отбросить прошлые кадры. Меняются сразу, без перезапуска."));
    }
    const bool upscaler_on = s.upscaler != BbSettings::UpscalerOff;
    const bool taa = s.upscaler == BbSettings::UpscalerTaa;
    ImGui::BeginDisabled(!upscaler_on);
    ImGui::BeginDisabled(taa);
    int preset = taa ? BbSettings::NativeAA : s.preset.load();
    char preset_label[64];
    std::snprintf(preset_label, sizeof(preset_label), "%s (x%.1f)", BbSettings::PresetName(preset),
                  BbSettings::PresetScale(preset));
    if (ImGui::BeginCombo(BbSettings::MenuText("Preset", "Пресет"), preset_label)) {
        for (int i = 0; i < BbSettings::PresetCount; ++i) {
            char label[64];
            const float scale = BbSettings::PresetScale(i);
            const auto output = BbSettings::CurrentOutput();
            std::snprintf(
                label, sizeof(label),
                BbSettings::MenuText("%s (x%.1f, render %dx%d)", "%s (x%.1f, рендер %dx%d)"),
                BbSettings::PresetName(i), scale, int(std::lround(output.width / scale / 2) * 2),
                int(std::lround(output.height / scale / 2) * 2));
            if (ImGui::Selectable(label, i == preset)) {
                Store(s.preset, i, true);
            }
        }
        ImGui::EndCombo();
    }
    ImGui::EndDisabled();
    if (taa) {
        ImGui::TextWrapped("%s", BbSettings::MenuText(
            "TAA anti-aliases the scene at output resolution, without an FSR model or upscaling. "
            "Your saved FSR preset is restored when you select FSR.",
            "TAA сглаживает сцену в разрешении вывода, без модели FSR и апскейлинга. "
            "Сохранённый пресет FSR восстановится при выборе FSR."));
    }
    ImGui::Text(
        BbSettings::MenuText("Active scene render: %d x %d", "Активный рендер сцены: %d x %d"),
        s.active_render_width.load(), s.active_render_height.load());
    Hint(BbSettings::MenuText(
        "Native AA: the upscaler acts as anti-aliasing. Other presets reduce the 3D G-Buffer and "
        "deferred lighting resolution immediately on the next frame (full Yebis2 post-FX rebuild "
        "applies on game restart).",
        "Native AA: апскейлер работает как сглаживание. Остальные пресеты уменьшают разрешение "
        "3D G-Buffer и освещения со следующего кадра (полная перестройка постобработки Yebis2 — "
        "после перезапуска)."));
    Checkbox(BbSettings::MenuText("Sharpening (RCAS)", "Резкость (RCAS)"), s.sharpen);
    ImGui::BeginDisabled(!s.sharpen);
    Slider(BbSettings::MenuText("Sharpness", "Сила резкости"), s.sharpness, 0.0f, 2.0f);
    Hint(BbSettings::MenuText(
        "Up to 1: the upscaler's own sharpening (RCAS). Above 1 adds another RCAS pass. "
        "Ctrl+click the slider to enter an exact value.",
        "До 1 — резкость самого апскейлера (RCAS). Выше 1 добавляется ещё один проход RCAS. "
        "Ctrl+клик по ползунку — ввести точное значение."));
    ImGui::EndDisabled();
    // Frame generation: hidden until its artifacts are fixed (BB_FRAME_GEN=1 for tests).
    if (std::getenv("BB_FRAME_GEN")) {
        ImGui::BeginDisabled(!s.frame_generation_supported.load(std::memory_order_relaxed));
        Checkbox(BbSettings::MenuText("Frame generation (FSR 3)", "Генерация кадров (FSR 3)"),
                 s.frame_generation);
        ImGui::EndDisabled();
        if (const char* problem = s.frame_generation_problem.load(std::memory_order_relaxed)) {
            Hint(problem);
        } else {
            Hint(BbSettings::MenuText(
                "Shows a frame interpolated by FSR 3 between each two of the game's: twice the frames "
                "on screen, a little more input lag. Works with every upscaler preset except Off.",
                "Между каждыми двумя кадрами игры показывается кадр, интерполированный FSR 3: на "
                "экране вдвое больше кадров, задержка ввода чуть выше. Работает со всеми пресетами "
                "апскейлера, кроме «Выкл»."));
        }
    }
    Checkbox(BbSettings::MenuText("Subpixel jitter", "Субпиксельный сдвиг (jitter)"), s.jitter);
    Hint(BbSettings::MenuText(
        "Each frame shifts the scene by a fraction of a pixel, letting the upscaler reconstruct "
        "more detail from multiple frames. Without it, only history-based anti-aliasing remains.",
        "Каждый кадр сцена сдвигается на долю пикселя, и апскейлер собирает из нескольких "
        "кадров больше деталей. Без него получается только сглаживание по истории."));

    ImGui::SeparatorText(BbSettings::MenuText("Reactive mask", "Маска реактивности"));
    ImGui::BeginDisabled(taa);
    Checkbox(BbSettings::MenuText("Enable mask", "Включить маску"), s.reactive);
    Hint(BbSettings::IsFsr4(s.upscaler)
             ? BbSettings::MenuText(
                   "Marks transparent effects (haze, light, water, particles). FSR 4 takes no "
                   "mask itself: its output is blended with the current frame there, so the "
                   "effects do not drag previous frames along.",
                   "Помечает прозрачные эффекты (дымку, свет, воду, частицы). FSR 4 сам маску не "
                   "принимает: там его результат смешивается с текущим кадром, и эффекты не "
                   "тянут за собой прошлые кадры.")
             : BbSettings::MenuText(
                   "Marks transparent effects (particles, haze) so the upscaler relies less on "
                   "previous frames. Reduces trails behind effects, but shimmering returns "
                   "underneath them.",
                   "Помечает прозрачные эффекты (частицы, дымку), чтобы апскейлер меньше "
                   "опирался на прошлые кадры. Меньше шлейфов за эффектами, но под ними "
                   "возвращается дрожание."));
    ImGui::BeginDisabled(!s.reactive);
    Slider(BbSettings::MenuText("Scale", "Масштаб"), s.reactive_scale, 0.0f, 4.0f);
    Slider(BbSettings::MenuText("Threshold", "Порог"), s.reactive_threshold, 0.0f, 1.0f);
    Slider(BbSettings::MenuText("Maximum", "Максимум"), s.reactive_max, 0.0f, 1.0f);
    bool show_mask = s.debug_view == BbSettings::DebugReactive;
    if (ImGui::Checkbox(BbSettings::MenuText("Show mask (debug)", "Показать маску (отладка)"),
                        &show_mask)) {
        s.debug_view = show_mask ? BbSettings::DebugReactive : BbSettings::DebugNone;
    }
    ImGui::EndDisabled();
    ImGui::EndDisabled();
    Checkbox(BbSettings::MenuText("Character motion vectors", "Векторы движения персонажей"),
             s.object_motion);
    Hint(BbSettings::MenuText(
        "Accurate vectors for animated objects: clothing and weapons break up less "
        "during movement. The static scene does not get an additional pass. "
        "Changes apply after restarting the game.",
        "Точные векторы для анимированных объектов: одежда и оружие меньше рассыпаются "
        "при движении. Статичная сцена не получает дополнительный проход. "
        "Изменение применяется после перезапуска игры."));
    bool show_motion = s.debug_view == BbSettings::DebugMotion;
    if (ImGui::Checkbox(BbSettings::MenuText("Show motion vectors (debug)",
                                             "Показать векторы движения (отладка)"),
                        &show_motion)) {
        s.debug_view = show_motion ? BbSettings::DebugMotion : BbSettings::DebugNone;
    }
    Hint(BbSettings::MenuText(
        "Red/green: horizontal/vertical motion (8 pixels = full brightness). "
        "Blue: the pixel received an accurate object vector, not just camera motion. "
        "The upscaler treats a moving object with no blue or red/green as "
        "stationary, causing trails.",
        "Красный/зелёный: движение по горизонтали/вертикали (8 пикселей = полная яркость). "
        "Синий: пиксель получил точный вектор объекта, а не только движение камеры. "
        "Движущийся предмет без синего и без красного/зелёного апскейлер считает "
        "неподвижным, отсюда шлейф."));
    ImGui::EndDisabled(); // upscaler off

    ImGui::SeparatorText(BbSettings::MenuText("Output", "Вывод"));
    // The aspect ratio first; its resolutions follow (another one keeps the height).
    const auto aspect_label = [](int a) {
        if (a != BbSettings::AspectAuto) return std::string(BbSettings::AspectKey(a));
        return std::string(BbSettings::MenuText("Auto", "Авто")) + " (" +
               BbSettings::AspectKey(BbSettings::ResolvedAspect(a)) + ")";
    };
    const int aspect = s.aspect;
    if (ImGui::BeginCombo(BbSettings::MenuText("Aspect ratio", "Соотношение сторон"),
                          aspect_label(aspect).c_str())) {
        for (int a = 0; a < BbSettings::AspectCount; ++a) {
            if (ImGui::Selectable(aspect_label(a).c_str(), a == aspect)) {
                Store(s.output_res, BbSettings::PackSize(BbSettings::NearestOutput(a, BbSettings::CurrentOutput())),
                      true);
                Store(s.aspect, a, true);
            }
        }
        ImGui::EndCombo();
    }
    const auto size_label = [](BbSettings::OutputSize o) {
        return std::to_string(o.width) + " x " + std::to_string(o.height);
    };
    const auto current = BbSettings::CurrentOutput();
    if (ImGui::BeginCombo(BbSettings::MenuText("Resolution", "Разрешение"), size_label(current).c_str())) {
        for (const auto& o : BbSettings::Outputs(s.aspect)) {
            if (ImGui::Selectable(size_label(o).c_str(), o == current)) {
                Store(s.output_res, BbSettings::PackSize(o), true);
            }
        }
        ImGui::EndCombo();
    }
    Hint(BbSettings::MenuText(
        "The output frame, UI, and 3D G-Buffer resolution update on the next frame boundary "
        "(full Yebis2 post-FX buffer reallocation applies on restart). Another aspect ratio "
        "widens the view while keeping UI anchored to screen edges.",
        "Размер кадра, интерфейса и 3D G-Buffer меняется на следующем кадре (полное "
        "перевыделение буферов постобработки Yebis2 — после перезапуска). Другое соотношение "
        "сторон расширяет обзор, оставляя интерфейс у краёв экрана."));
    ImGui::SeparatorText(BbSettings::MenuText("Game effects & Texture filtering",
                                              "Эффекты игры и фильтрация текстур"));
    {
        const char* aniso_labels[] = {
            BbSettings::MenuText("Off (PS4 Default)", "Выкл (Как на PS4)"),
            BbSettings::MenuText("4x (Medium)", "4x (Средняя)"),
            BbSettings::MenuText("8x (High)", "8x (Высокая)"),
            BbSettings::MenuText("16x (Ultra)", "16x (Ультра)")};
        static constexpr int aniso_values[] = {1, 4, 8, 16};
        const int cur_aniso = s.anisotropic.load();
        int aniso_idx = cur_aniso >= 16 ? 3 : cur_aniso >= 8 ? 2 : cur_aniso >= 4 ? 1 : 0;
        if (ImGui::Combo(BbSettings::MenuText("Anisotropic filtering", "Анизотропная фильтрация"),
                         &aniso_idx, aniso_labels, 4)) {
            Store(s.anisotropic, aniso_values[aniso_idx], true);
        }
    }
    const char* lods[] = {BbSettings::MenuText("Highest (-2)", "Максимальная (-2)"),
                          BbSettings::MenuText("Game default", "Как в игре"),
                          BbSettings::MenuText("Lower (1)", "Ниже (1)"),
                          BbSettings::MenuText("Lowest (2)", "Минимальная (2)")};
    static constexpr int lod_values[] = {-2, 0, 1, 2};
    int lod_index = 1;
    for (int i = 0; i < 4; ++i) {
        if (lod_values[i] == s.model_lod) lod_index = i;
    }
    if (ImGui::BeginCombo(BbSettings::MenuText("Model detail", "Детализация моделей"),
                          lods[lod_index])) {
        for (int i = 0; i < 4; ++i) {
            if (ImGui::Selectable(lods[i], i == lod_index)) {
                Store(s.model_lod, lod_values[i], true);
            }
        }
        ImGui::EndCombo();
    }
    for (int e = 0; e < BbSettings::EffectCount; ++e) {
        if (e == 4) continue; // effect_game_aa is controlled by the Anti-Aliasing combo above
        const auto& effect = BbSettings::Effects[e];
        Checkbox(BbSettings::MenuText(effect.label, effect.label_ru), s.effects[e]);
        if (std::string_view(effect.key) == "debug_camera") {
            Hint(BbSettings::MenuText("Hold Cross and press L3 (keyboard: F + Left Ctrl).",
                                      "Удерживайте Cross и нажмите L3 (клавиатура: F + Left Ctrl)."));
        } else if (std::string_view(effect.key) == "debug_menu") {
            Hint(BbSettings::MenuText(
                "Left side of the touchpad (Tab); right side: Backspace. Needs the adhoc folder "
                "from Nexus mod #253 (the fonts in adhoc/font).",
                "Левая сторона тачпада (Tab), правая — Backspace. Нужна папка adhoc из мода "
                "Nexus #253 (шрифты в adhoc/font)."));
        }
    }
    BbGuestHooks::ApplyGameEffects();
    Hint(BbSettings::MenuText(
        "All visual effects (including SSR reflections, Bloom, DoF, Motion Blur, SSAO, and Model LOD) "
        "switch live in-game. Skip intro videos, free camera, and debug menu apply on startup.",
        "Все графические эффекты (включая отражения SSR, Bloom, DoF, Motion Blur, SSAO и детализацию "
        "моделей) переключаются в игре на лету. Пропуск заставок, свободная камера и debug menu "
        "применяются при запуске."));

    bool restart =
        s.object_motion != s.startup_object_motion || BbSettings::ResolutionNeedsRestart();
    for (int e = 0; e < BbSettings::EffectCount; ++e) {
        restart |= !BbSettings::IsHotEffect(e) && s.effects[e] != s.startup_effects[e];
    }
    if (restart) {
        ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.3f, 1.0f), "%s",
                           BbSettings::MenuText("Changes apply after restarting the game",
                                                "Изменения применятся после перезапуска игры"));
        if (ImGui::Button(
                BbSettings::MenuText("Apply and restart game", "Применить и перезапустить игру"))) {
            BbSettings::Save();
            runtime_restart();
        }
    }

    ImGui::SeparatorText(BbSettings::MenuText("Frame rate limit", "Лимит кадров (FPS)"));
    {
        static const int steps[] = {0, 30, 60, 90, 120, 144, 165, 170, 240, 360, 480, 1000, 2000, 5000};
        static const char* labels[] = {"Uncapped (Unlimited)", "30", "60", "90", "120", "144", "165", "170", "240", "360", "480", "1000", "2000", "5000"};
        static const char* labels_ru[] = {"Без лимита (Анлимит)", "30", "60", "90", "120", "144", "165", "170", "240", "360", "480", "1000", "2000", "5000"};
        constexpr int step_count = int(sizeof(steps) / sizeof(steps[0]));
        const int cur_limit = BbFrameRate::Limit();
        int index = -1;
        for (int i = 0; i < step_count; ++i) {
            if (steps[i] == cur_limit) index = i;
        }
        char combo_preview[48];
        if (index >= 0) {
            std::snprintf(combo_preview, sizeof(combo_preview), "%s",
                          BbSettings::MenuText(labels[index], labels_ru[index]));
        } else {
            std::snprintf(combo_preview, sizeof(combo_preview),
                          BbSettings::MenuText("%d FPS (Custom)", "%d FPS (Свой)"), cur_limit);
        }
        if (ImGui::BeginCombo(BbSettings::MenuText("Preset (F3: 30 FPS toggle)", "Пресет (F3: 30 FPS вкл/выкл)"),
                              combo_preview)) {
            for (int i = 0; i < step_count; ++i) {
                if (ImGui::Selectable(BbSettings::MenuText(labels[i], labels_ru[i]), i == index)) {
                    Store(s.fps_limit, steps[i], true);
                    BbFrameRate::SetLimit(steps[i]);
                }
            }
            ImGui::EndCombo();
        }
        int custom_fps = cur_limit;
        if (ImGui::SliderInt(BbSettings::MenuText("Custom FPS (Ctrl+click to type)",
                                                  "Точный FPS (Ctrl+клик — ввести число)"),
                             &custom_fps, 0, 5000, custom_fps == 0 ? "Uncapped (Unlimited)" : "%d FPS")) {
            if (custom_fps > 0 && custom_fps < 30) custom_fps = 30;
            Store(s.fps_limit, custom_fps, true);
            BbFrameRate::SetLimit(custom_fps);
        }
        Hint(BbSettings::MenuText(
            "Saved in bbport.ini. Set 0 for uncapped frame rate, or Ctrl+click the slider to type "
            "any exact target (e.g. 165, 170, 180). At 30 FPS the cinematic of Laurence does not hang.",
            "Сохраняется в bbport.ini. 0 — без лимита, либо Ctrl+клик по ползунку для ввода любого "
            "точного числа (например, 165, 170, 180). На 30 FPS кинематика Лоуренса не зависает."));
    }
    ImGui::SeparatorText(BbSettings::MenuText("Controls (Keyboard & Mouse)", "Управление (Клавиатура и мышь)"));
    if (ImGui::CollapsingHeader(
            BbSettings::MenuText("Keybindings & Mouse Camera (Live)", "Горячие клавиши и мышь (на лету)"),
            ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextWrapped("%s", BbSettings::MenuText(
            "Movement: WASD  |  Walk: Left Ctrl + WASD  |  Jump: hold Sprint + press Dodge\n"
            "Switch weapons: 1 (Left hand), 2 (Right hand)  |  Switch items/magic: Mouse Wheel or 3 / 4\n"
            "Personal effects (Right Touchpad): Backspace  |  Release mouse cursor: F1",
            "Движение: WASD  |  Шаг: Left Ctrl + WASD  |  Прыжок: удерживать Бег + нажать Уклонение\n"
            "Смена оружия: 1 (левая рука), 2 (правая рука)  |  Смена предметов: Колесо мыши или 3 / 4\n"
            "Личные вещи (правый тачпад): Backspace  |  Освободить курсор мыши: F1"));
        ImGui::Spacing();
        Checkbox(BbSettings::MenuText("Mouse camera look", "Обзор камерой мыши"), s.mouse_look);
        ImGui::SameLine();
        Checkbox(BbSettings::MenuText("Invert Y axis", "Инверсия оси Y"), s.mouse_invert_y);
        ImGui::BeginDisabled(!s.mouse_look);
        Slider(BbSettings::MenuText("Mouse sensitivity", "Чувствительность мыши"), s.mouse_sensitivity, 0.10f, 4.00f);
        ImGui::EndDisabled();
        ImGui::Spacing();

        if (rebinding_action >= 0 && SDL_WasInit(SDL_INIT_VIDEO)) {
            int num_keys = 0;
            const bool* keys = SDL_GetKeyboardState(&num_keys);
            const SDL_MouseButtonFlags mb = SDL_GetMouseState(nullptr, nullptr);
            const bool esc_down = keys && num_keys > SDL_SCANCODE_ESCAPE && keys[SDL_SCANCODE_ESCAPE];
            std::vector<std::string> down;
            if (keys) {
                for (int sc = 4; sc < std::min(num_keys, int(SDL_SCANCODE_COUNT)); ++sc) {
                    if (sc == SDL_SCANCODE_ESCAPE || sc == SDL_SCANCODE_INSERT || !keys[sc]) continue;
                    const char* raw_name = SDL_GetScancodeName(SDL_Scancode(sc));
                    if (!raw_name || !*raw_name) continue;
                    std::string name = raw_name;
                    if (name == "Return") name = "Enter";
                    down.push_back(std::move(name));
                }
            }
            if (mb & SDL_BUTTON_LMASK) down.emplace_back("Mouse Left");
            if (mb & SDL_BUTTON_RMASK) down.emplace_back("Mouse Right");
            if (mb & SDL_BUTTON_MMASK) down.emplace_back("Mouse Middle");
            if (mb & SDL_BUTTON_X1MASK) down.emplace_back("Mouse 4");
            if (mb & SDL_BUTTON_X2MASK) down.emplace_back("Mouse 5");

            if (rebind_wait_initial_release) {
                if (!esc_down && down.empty()) {
                    rebind_wait_initial_release = false;
                }
            } else if (esc_down) {
                rebind_tokens.clear();
                rebinding_action = -1;
            } else {
                for (const std::string& tok : down) {
                    if (rebind_tokens.size() < 3 &&
                        std::find(rebind_tokens.begin(), rebind_tokens.end(), tok) == rebind_tokens.end()) {
                        rebind_tokens.push_back(tok);
                    }
                }
                if (down.empty() && !rebind_tokens.empty()) {
                    auto is_mod = [](const std::string& t) {
                        return t == "Left Shift" || t == "Right Shift" || t == "Left Ctrl" ||
                               t == "Right Ctrl" || t == "Left Alt" || t == "Right Alt";
                    };
                    std::stable_partition(rebind_tokens.begin(), rebind_tokens.end(), is_mod);
                    std::string bound;
                    for (std::size_t i = 0; i < rebind_tokens.size(); ++i) {
                        if (i > 0) bound += "+";
                        bound += rebind_tokens[i];
                    }
                    BbSettings::SetKeyBinding(rebinding_action, bound);
                    dirty = true;
                    BbSettings::Save();
                    rebind_tokens.clear();
                    rebinding_action = -1;
                }
            }
        }

        for (int k = 0; k < BbSettings::KeyActionCount; ++k) {
            ImGui::PushID(k);
            const auto& def = BbSettings::GetKeyActionDef(k);
            const std::string cur = BbSettings::FormatKeyBinding(BbSettings::GetKeyBinding(k));
            const bool waiting = (rebinding_action == k);
            std::string waiting_label;
            if (waiting) {
                if (!rebind_tokens.empty()) {
                    waiting_label = "[ ";
                    for (std::size_t i = 0; i < rebind_tokens.size(); ++i) {
                        if (i > 0) waiting_label += " + ";
                        waiting_label += rebind_tokens[i];
                    }
                    waiting_label += " ... ]";
                } else {
                    waiting_label = BbSettings::MenuText("[ Press key(s)... (Esc: cancel) ]",
                                                         "[ Нажмите клавишу... (Esc: отмена) ]");
                }
            }
            const char* btn_label = waiting ? waiting_label.c_str() : cur.c_str();
            if (waiting) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.45f, 0.10f, 1.0f));
            }
            if (ImGui::Button(btn_label, ImVec2(230.0f * base_scale, 0.0f))) {
                if (waiting) {
                    rebinding_action = -1;
                    rebind_tokens.clear();
                } else {
                    rebinding_action = k;
                    rebind_wait_initial_release = true;
                    rebind_tokens.clear();
                }
            }
            if (waiting) {
                ImGui::PopStyleColor();
            }
            ImGui::SameLine();
            ImGui::TextUnformatted(BbSettings::MenuText(def.name_en, def.name_ru));
            Hint(BbSettings::MenuText(def.help_en, def.help_ru));
            ImGui::PopID();
        }
        ImGui::Spacing();
        if (ImGui::Button(BbSettings::MenuText("Reset controls to defaults", "Сбросить клавиши по умолчанию"))) {
            rebinding_action = -1;
            rebind_tokens.clear();
            BbSettings::ResetKeyBindingsToDefault();
            dirty = true;
            BbSettings::Save();
        }
    }

    ImGui::SeparatorText(BbSettings::MenuText("Other", "Прочее"));
    Checkbox(BbSettings::MenuText("Debug & Pipeline Telemetry Overlay (FPS / DirectVk Stats)",
                                  "Оверлей телеметрии и отладки (FPS / Статистика DirectVk)"),
             s.show_fps);
    if (s.show_fps) {
        ImGui::SameLine();
        if (ImGui::SmallButton(BbSettings::MenuText("Reset Avg", "Сбросить Avg"))) {
            reset_hud_avg.store(true, std::memory_order_relaxed);
        }
    }

    ImGui::Spacing();
    if (ImGui::Button(BbSettings::MenuText("Close", "Закрыть"))) {
        keep_open = false;
    }
    // Issues #74, #116: leaving the game from the pad (this menu opens with L3+R3). A second
    // press confirms; the window thread then closes like the window's own close button.
    static bool quit_armed;
    ImGui::SameLine();
    if (ImGui::Button(quit_armed ? BbSettings::MenuText("Press again to quit", "Нажмите ещё раз для выхода")
                                 : BbSettings::MenuText("Quit game", "Выйти из игры"))) {
        if (quit_armed) {
            SDL_Event quit{};
            quit.type = SDL_EVENT_QUIT;
            SDL_PushEvent(&quit);
        }
        quit_armed = !quit_armed;
    }
    if (quit_armed && !ImGui::IsItemFocused() && !ImGui::IsItemHovered()) {
        quit_armed = false;
    }
    ImGui::TextDisabled("%s", BbSettings::MenuText("Settings are saved to bbport.ini",
                                             "Настройки сохраняются в bbport.ini"));
    ImGui::End();
    if (!keep_open) {
        SetOpen(false);
    }
}

struct HudSampleSlot {
    std::uint64_t frames = 0;
    double time_s = 0.0;
    double present_ms_sum = 0.0;
    double present_ms_sq_sum = 0.0;
    std::uint64_t present_ms_count = 0;
    std::uint64_t draws = 0;
    std::uint64_t dispatches = 0;
    std::uint64_t submits = 0;
    std::uint64_t faults = 0;
    std::uint64_t native_buf_hits = 0;
    std::uint64_t native_buf_total = 0;
    std::uint64_t native_tex_hits = 0;
    std::uint64_t native_tex_total = 0;
    std::uint64_t desc_saved = 0;
    std::uint64_t desc_total = 0;
    double idle_a_ms = 0.0;
    double gnm_wait_ms = 0.0;
    double a_pm4_pipe_ms = 0.0;
    double a_ring_ms = 0.0;
    double a_drain_ms = 0.0;
    double b_total_ms = 0.0;
    double b_buffers_ms = 0.0;
    double b_tex_desc_ms = 0.0;
    double b_state_pass_ms = 0.0;
    double b_tasks_ms = 0.0;
    double b_packet_misc_ms = 0.0;
    double gpu_wait_ms = 0.0;
    double hw_gpu_busy_ms = 0.0;
    double hw_gpu_gap_ms = 0.0;
    std::uint64_t hw_gpu_frames = 0;
};

// Sliding ~15-second ring buffer (44 * 0.35s = 15.4s, O(1) update, zero heap allocations).
struct HudSlidingWindow {
    static constexpr std::size_t kSlots = 44;
    std::array<HudSampleSlot, kSlots> slots{};
    HudSampleSlot sum{};
    std::size_t head = 0;
    std::size_t count = 0;

    void Reset() noexcept { *this = HudSlidingWindow{}; }

    void Push(const HudSampleSlot& s) noexcept {
        if (count == kSlots) {
            const HudSampleSlot& old = slots[head];
            auto sub_u64 = [](std::uint64_t& acc, std::uint64_t v) { acc = acc >= v ? acc - v : 0; };
            auto sub_f64 = [](double& acc, double v) { acc = std::max(0.0, acc - v); };
            sub_u64(sum.frames, old.frames);
            sub_f64(sum.time_s, old.time_s);
            sub_f64(sum.present_ms_sum, old.present_ms_sum);
            sub_f64(sum.present_ms_sq_sum, old.present_ms_sq_sum);
            sub_u64(sum.present_ms_count, old.present_ms_count);
            sub_u64(sum.draws, old.draws);
            sub_u64(sum.dispatches, old.dispatches);
            sub_u64(sum.submits, old.submits);
            sub_u64(sum.faults, old.faults);
            sub_u64(sum.native_buf_hits, old.native_buf_hits);
            sub_u64(sum.native_buf_total, old.native_buf_total);
            sub_u64(sum.native_tex_hits, old.native_tex_hits);
            sub_u64(sum.native_tex_total, old.native_tex_total);
            sub_u64(sum.desc_saved, old.desc_saved);
            sub_u64(sum.desc_total, old.desc_total);
            sub_f64(sum.idle_a_ms, old.idle_a_ms);
            sub_f64(sum.gnm_wait_ms, old.gnm_wait_ms);
            sub_f64(sum.a_pm4_pipe_ms, old.a_pm4_pipe_ms);
            sub_f64(sum.a_ring_ms, old.a_ring_ms);
            sub_f64(sum.a_drain_ms, old.a_drain_ms);
            sub_f64(sum.b_total_ms, old.b_total_ms);
            sub_f64(sum.b_buffers_ms, old.b_buffers_ms);
            sub_f64(sum.b_tex_desc_ms, old.b_tex_desc_ms);
            sub_f64(sum.b_state_pass_ms, old.b_state_pass_ms);
            sub_f64(sum.b_tasks_ms, old.b_tasks_ms);
            sub_f64(sum.b_packet_misc_ms, old.b_packet_misc_ms);
            sub_f64(sum.gpu_wait_ms, old.gpu_wait_ms);
            sub_f64(sum.hw_gpu_busy_ms, old.hw_gpu_busy_ms);
            sub_f64(sum.hw_gpu_gap_ms, old.hw_gpu_gap_ms);
            sub_u64(sum.hw_gpu_frames, old.hw_gpu_frames);
        } else {
            ++count;
        }
        slots[head] = s;
        head = (head + 1) % kSlots;
        sum.frames += s.frames;
        sum.time_s += s.time_s;
        sum.present_ms_sum += s.present_ms_sum;
        sum.present_ms_sq_sum += s.present_ms_sq_sum;
        sum.present_ms_count += s.present_ms_count;
        sum.draws += s.draws;
        sum.dispatches += s.dispatches;
        sum.submits += s.submits;
        sum.faults += s.faults;
        sum.native_buf_hits += s.native_buf_hits;
        sum.native_buf_total += s.native_buf_total;
        sum.native_tex_hits += s.native_tex_hits;
        sum.native_tex_total += s.native_tex_total;
        sum.desc_saved += s.desc_saved;
        sum.desc_total += s.desc_total;
        sum.idle_a_ms += s.idle_a_ms;
        sum.gnm_wait_ms += s.gnm_wait_ms;
        sum.a_pm4_pipe_ms += s.a_pm4_pipe_ms;
        sum.a_ring_ms += s.a_ring_ms;
        sum.a_drain_ms += s.a_drain_ms;
        sum.b_total_ms += s.b_total_ms;
        sum.b_buffers_ms += s.b_buffers_ms;
        sum.b_tex_desc_ms += s.b_tex_desc_ms;
        sum.b_state_pass_ms += s.b_state_pass_ms;
        sum.b_tasks_ms += s.b_tasks_ms;
        sum.b_packet_misc_ms += s.b_packet_misc_ms;
        sum.gpu_wait_ms += s.gpu_wait_ms;
        sum.hw_gpu_busy_ms += s.hw_gpu_busy_ms;
        sum.hw_gpu_gap_ms += s.hw_gpu_gap_ms;
        sum.hw_gpu_frames += s.hw_gpu_frames;
    }
};

struct HudBreakdown {
    float fps = 0.0f;
    float frame_ms = 0.0f;
    float avg_fps = 0.0f;
    float avg_frame_ms = 0.0f;
    float jitter_ms = 0.0f;
    float avg_jitter_ms = 0.0f;
    float window_s = 0.0f;
    bool has_avg = false;
    float b_busy_pct = 0.0f, avg_b_busy_pct = 0.0f;
    float draws_pf = 0.0f, avg_draws_pf = 0.0f;
    float dispatches_pf = 0.0f, avg_dispatches_pf = 0.0f;
    float submits_pf = 0.0f, avg_submits_pf = 0.0f;
    float drains_pf = 0.0f;
    float faults_ps = 0.0f, avg_faults_ps = 0.0f;
    float eop_pf = 0.0f;
    float native_buf_pct = 0.0f, avg_native_buf_pct = 0.0f;
    float native_tex_pct = 0.0f, avg_native_tex_pct = 0.0f;
    float desc_saved_pct = 0.0f, avg_desc_saved_pct = 0.0f;
    // Stage 1: Game CPU / Gnm
    float idle_a_ms = 0.0f, avg_idle_a_ms = 0.0f;
    float gnm_wait_ms = 0.0f, avg_gnm_wait_ms = 0.0f;
    // Stage 2: Stage A (bb:GpuComm)
    float a_total_ms = 0.0f, avg_a_total_ms = 0.0f;
    float a_pm4_pipe_ms = 0.0f, avg_a_pm4_pipe_ms = 0.0f;
    float a_ring_ms = 0.0f, avg_a_ring_ms = 0.0f;
    float a_drain_ms = 0.0f, avg_a_drain_ms = 0.0f;
    float a_flush_ms = 0.0f;
    // Stage 3: Stage B (bb:DrawRec)
    float b_total_ms = 0.0f, avg_b_total_ms = 0.0f;
    float b_buffers_ms = 0.0f, avg_b_buffers_ms = 0.0f;
    float b_obtain_ms = 0.0f;
    float b_vram_ms = 0.0f;
    float b_tex_desc_ms = 0.0f, avg_b_tex_desc_ms = 0.0f;
    float b_state_pass_ms = 0.0f, avg_b_state_pass_ms = 0.0f;
    float b_motion_hle_ms = 0.0f;
    float b_tasks_ms = 0.0f, avg_b_tasks_ms = 0.0f;
    float b_packet_misc_ms = 0.0f, avg_b_packet_misc_ms = 0.0f;
    float b_flush_ms = 0.0f;
    // Stage 4: GPU / Sync Wait + Hardware GPU Busy/Gap
    float gpu_present_ms = 0.0f;
    float sync_rec_ms = 0.0f;
    float host_copy_ms = 0.0f;
    float avg_gpu_wait_ms = 0.0f;
    bool has_hw_gpu = false;
    float hw_gpu_busy_ms = 0.0f, avg_hw_gpu_busy_ms = 0.0f;
    float hw_gpu_gap_ms = 0.0f, avg_hw_gpu_gap_ms = 0.0f;
    // Bottleneck summary
    const char* bottleneck_ru = "Ожидание данных...";
    const char* bottleneck_en = "Collecting data...";
    float bottleneck_ms = 0.0f;
    float avg_bottleneck_ms = 0.0f;
    int fps_cap_active = 0;
    int bottleneck_kind = 0; // 0=gpu, 1=game cpu, 2=stage A, 3=stage B, 4=gnm serialized, 5=fps cap
};

void UpdateHudBreakdown(HudBreakdown& h) {
    using Clock = std::chrono::steady_clock;
    static bool first = true;
    static Clock::time_point last_time{};
    static Clock::time_point last_file_dump{};
    static std::uint64_t last_tsc = 0;
    static std::uint64_t last_frames = 0;
    static std::uint64_t last_draws = 0, last_dispatches = 0, last_submits = 0;
    static std::uint64_t last_idle_ns = 0, last_gnm_wait_ns = 0;
    static std::uint64_t last_present_ns = 0, last_sync_rec_ns = 0, last_host_copy_ns = 0;
    static std::uint64_t last_pipe_drains = 0, last_pipe_drain_cyc = 0, last_pipe_busy_cyc = 0;
    static std::uint64_t last_faults = 0, last_eop = 0;
    static std::uint64_t last_native_hits = 0, last_native_misses = 0;
    static std::uint64_t last_native_tex_hits = 0, last_native_tex_misses = 0;
    static std::uint64_t last_desc_saved = 0, last_desc_total = 0;
    static std::uint64_t last_hw_busy_ns = 0, last_hw_gap_ns = 0, last_hw_frames = 0;
    static std::array<std::uint64_t, BbSections::Count> last_ca{}, last_cb{};

    const auto now = Clock::now();
    const std::uint64_t tsc = __rdtsc();
    const std::uint64_t cur_frames = BbStats::frame_number.load(std::memory_order_relaxed);
    if (first) {
        first = false;
        last_time = now;
        last_file_dump = now;
        last_tsc = tsc;
        last_frames = cur_frames;
        last_draws = BbStats::draws.load(std::memory_order_relaxed);
        last_dispatches = BbStats::dispatches.load(std::memory_order_relaxed);
        last_submits = Vulkan::Scheduler::recorded_submissions.load(std::memory_order_relaxed);
        last_idle_ns = BbStats::gpu_idle_ns.load(std::memory_order_relaxed);
        last_gnm_wait_ns = BbStats::gnm_frame_wait_ns.load(std::memory_order_relaxed);
        last_present_ns = BbStats::present_wait_ns.load(std::memory_order_relaxed);
        last_sync_rec_ns = BbStats::sync_recording_ns.load(std::memory_order_relaxed) +
                           BbStats::tick_wait_ns.load(std::memory_order_relaxed);
        last_host_copy_ns = BbStats::host_copies_wait_ns.load(std::memory_order_relaxed) +
                            BbStats::copy_threads_wait_ns.load(std::memory_order_relaxed);
        last_pipe_drains = BbSections::pipe_drains.load(std::memory_order_relaxed);
        last_pipe_drain_cyc = BbSections::pipe_drain_cycles.load(std::memory_order_relaxed);
        last_pipe_busy_cyc = BbSections::pipe_busy_cycles.load(std::memory_order_relaxed);
        last_faults = BbStats::tracker_faults_total.load(std::memory_order_relaxed);
        last_eop = BbStats::legacy_eop_decoded.load(std::memory_order_relaxed);
        last_native_hits = BbStats::native_buf_hits.load(std::memory_order_relaxed);
        last_native_misses = BbStats::native_buf_misses.load(std::memory_order_relaxed);
        last_native_tex_hits = BbStats::native_tex_hits.load(std::memory_order_relaxed);
        last_native_tex_misses = BbStats::native_tex_misses.load(std::memory_order_relaxed);
        last_desc_saved = BbStats::desc_push_saved.load(std::memory_order_relaxed);
        last_desc_total = BbStats::desc_push_total.load(std::memory_order_relaxed);
        last_hw_busy_ns = BbStats::hw_gpu_busy_ns.load(std::memory_order_relaxed);
        last_hw_gap_ns = BbStats::hw_gpu_gap_ns.load(std::memory_order_relaxed);
        last_hw_frames = BbStats::hw_gpu_frames.load(std::memory_order_relaxed);
        for (std::size_t i = 0; i < BbSections::Count; ++i) {
            last_ca[i] = BbSections::cycles_a[i].load(std::memory_order_relaxed);
            last_cb[i] = BbSections::cycles_b[i].load(std::memory_order_relaxed);
        }
        present_ms_sum_accum = 0.0;
        present_ms_sq_sum_accum = 0.0;
        present_ms_count_accum = 0;
        return;
    }

    const double dt = std::chrono::duration<double>(now - last_time).count();
    if (dt < 0.35) {
        return;
    }
    const std::uint64_t df = cur_frames > last_frames ? cur_frames - last_frames : 1;
    const double inv_f = 1.0 / double(df);
    const std::uint64_t dtsc = tsc > last_tsc ? tsc - last_tsc : 1;
    const double cyc_per_ms = double(dtsc) / (dt * 1000.0);
    const auto cyc_to_ms = [&](std::uint64_t c) -> float {
        return cyc_per_ms > 0.0 ? float((double(c) / cyc_per_ms) * inv_f) : 0.0f;
    };
    const auto ns_to_ms = [&](std::uint64_t ns) -> float {
        return float((double(ns) * 1e-6) * inv_f);
    };

    const std::uint64_t cur_draws = BbStats::draws.load(std::memory_order_relaxed);
    const std::uint64_t cur_dispatches = BbStats::dispatches.load(std::memory_order_relaxed);
    const std::uint64_t cur_submits =
        Vulkan::Scheduler::recorded_submissions.load(std::memory_order_relaxed);
    const std::uint64_t cur_idle_ns = BbStats::gpu_idle_ns.load(std::memory_order_relaxed);
    const std::uint64_t cur_gnm_wait_ns = BbStats::gnm_frame_wait_ns.load(std::memory_order_relaxed);
    const std::uint64_t cur_present_ns = BbStats::present_wait_ns.load(std::memory_order_relaxed);
    const std::uint64_t cur_sync_rec_ns = BbStats::sync_recording_ns.load(std::memory_order_relaxed) +
                                          BbStats::tick_wait_ns.load(std::memory_order_relaxed);
    const std::uint64_t cur_host_copy_ns =
        BbStats::host_copies_wait_ns.load(std::memory_order_relaxed) +
        BbStats::copy_threads_wait_ns.load(std::memory_order_relaxed);
    const std::uint64_t cur_pipe_drains = BbSections::pipe_drains.load(std::memory_order_relaxed);
    const std::uint64_t cur_pipe_drain_cyc =
        BbSections::pipe_drain_cycles.load(std::memory_order_relaxed);
    const std::uint64_t cur_pipe_busy_cyc =
        BbSections::pipe_busy_cycles.load(std::memory_order_relaxed);
    const std::uint64_t cur_faults =
        BbStats::tracker_faults_total.load(std::memory_order_relaxed);
    const std::uint64_t cur_eop = BbStats::legacy_eop_decoded.load(std::memory_order_relaxed);
    const std::uint64_t cur_native_hits =
        BbStats::native_buf_hits.load(std::memory_order_relaxed);
    const std::uint64_t cur_native_misses =
        BbStats::native_buf_misses.load(std::memory_order_relaxed);
    const std::uint64_t cur_native_tex_hits =
        BbStats::native_tex_hits.load(std::memory_order_relaxed);
    const std::uint64_t cur_native_tex_misses =
        BbStats::native_tex_misses.load(std::memory_order_relaxed);
    const std::uint64_t cur_desc_saved =
        BbStats::desc_push_saved.load(std::memory_order_relaxed);
    const std::uint64_t cur_desc_total =
        BbStats::desc_push_total.load(std::memory_order_relaxed);
    const std::uint64_t cur_hw_busy_ns =
        BbStats::hw_gpu_busy_ns.load(std::memory_order_relaxed);
    const std::uint64_t cur_hw_gap_ns =
        BbStats::hw_gpu_gap_ns.load(std::memory_order_relaxed);
    const std::uint64_t cur_hw_frames =
        BbStats::hw_gpu_frames.load(std::memory_order_relaxed);

    const auto safe_delta = [](std::uint64_t cur, std::uint64_t last) -> std::uint64_t {
        return cur >= last ? (cur - last) : cur;
    };

    std::array<std::uint64_t, BbSections::Count> dca{}, dcb{};
    for (std::size_t i = 0; i < BbSections::Count; ++i) {
        const std::uint64_t ca = BbSections::cycles_a[i].load(std::memory_order_relaxed);
        const std::uint64_t cb = BbSections::cycles_b[i].load(std::memory_order_relaxed);
        dca[i] = safe_delta(ca, last_ca[i]);
        dcb[i] = safe_delta(cb, last_cb[i]);
        last_ca[i] = ca;
        last_cb[i] = cb;
    }

    const std::uint64_t d_draws = safe_delta(cur_draws, last_draws);
    const std::uint64_t d_dispatches = safe_delta(cur_dispatches, last_dispatches);
    const std::uint64_t d_submits = safe_delta(cur_submits, last_submits);
    const std::uint64_t d_faults = safe_delta(cur_faults, last_faults);
    const std::uint64_t dh = safe_delta(cur_native_hits, last_native_hits);
    const std::uint64_t dm = safe_delta(cur_native_misses, last_native_misses);
    const std::uint64_t dth = safe_delta(cur_native_tex_hits, last_native_tex_hits);
    const std::uint64_t dtm = safe_delta(cur_native_tex_misses, last_native_tex_misses);
    const std::uint64_t dds = safe_delta(cur_desc_saved, last_desc_saved);
    const std::uint64_t ddt = safe_delta(cur_desc_total, last_desc_total);

    h.fps = float(double(df) / dt);
    h.frame_ms = float((dt * 1000.0) * inv_f);
    if (present_ms_count_accum > 1) {
        const double mean_p = present_ms_sum_accum / double(present_ms_count_accum);
        const double var_p =
            std::max(0.0, (present_ms_sq_sum_accum / double(present_ms_count_accum)) - mean_p * mean_p);
        h.jitter_ms = float(std::sqrt(var_p));
    } else {
        h.jitter_ms = 0.0f;
    }
    h.draws_pf = float(double(d_draws) * inv_f);
    h.dispatches_pf = float(double(d_dispatches) * inv_f);
    h.submits_pf = float(double(d_submits) * inv_f);
    h.drains_pf = float(double(safe_delta(cur_pipe_drains, last_pipe_drains)) * inv_f);
    h.faults_ps = float(double(d_faults) / dt);
    h.eop_pf = float(double(safe_delta(cur_eop, last_eop)) * inv_f);
    h.native_buf_pct = (dh + dm) > 0 ? float(100.0 * double(dh) / double(dh + dm)) : 0.0f;
    h.native_tex_pct = (dth + dtm) > 0 ? float(100.0 * double(dth) / double(dth + dtm)) : 0.0f;
    h.desc_saved_pct = ddt > 0 ? float(100.0 * double(dds) / double(ddt)) : 0.0f;

    const std::uint64_t d_hw_frames = safe_delta(cur_hw_frames, last_hw_frames);
    if (d_hw_frames > 0) {
        h.has_hw_gpu = true;
        h.hw_gpu_busy_ms = float(double(safe_delta(cur_hw_busy_ns, last_hw_busy_ns)) * 1e-6 /
                                 double(d_hw_frames));
        h.hw_gpu_gap_ms =
            float(double(safe_delta(cur_hw_gap_ns, last_hw_gap_ns)) * 1e-6 / double(d_hw_frames));
    }

    h.idle_a_ms = ns_to_ms(safe_delta(cur_idle_ns, last_idle_ns));
    h.gnm_wait_ms = ns_to_ms(safe_delta(cur_gnm_wait_ns, last_gnm_wait_ns));

    const float a_decode_ms = cyc_to_ms(dca[BbSections::ADecode]);
    h.a_ring_ms = cyc_to_ms(dca[BbSections::ARing]);
    h.a_drain_ms = cyc_to_ms(safe_delta(cur_pipe_drain_cyc, last_pipe_drain_cyc));
    h.a_flush_ms = cyc_to_ms(dca[BbSections::Flush]);
    h.a_pm4_pipe_ms = std::max(0.0f, a_decode_ms - h.a_ring_ms);
    h.a_total_ms = h.a_pm4_pipe_ms + h.a_ring_ms + h.a_drain_ms;

    h.gpu_present_ms = ns_to_ms(safe_delta(cur_present_ns, last_present_ns));
    const float raw_sync_rec_ms = ns_to_ms(safe_delta(cur_sync_rec_ns, last_sync_rec_ns));
    h.sync_rec_ms = std::max(0.0f, raw_sync_rec_ms - h.gpu_present_ms);
    h.host_copy_ms = ns_to_ms(safe_delta(cur_host_copy_ns, last_host_copy_ns));

    const float raw_b_total_ms = cyc_to_ms(safe_delta(cur_pipe_busy_cyc, last_pipe_busy_cyc));
    h.b_total_ms = std::max(0.0f, raw_b_total_ms - h.gpu_present_ms);
    const std::uint64_t b_vert_idx_cyc =
        dcb[BbSections::ResolveVertexBuffers] + dcb[BbSections::ResolveIndexBuffer];
    h.b_obtain_ms = cyc_to_ms(dcb[BbSections::ObtainBuffer]);
    h.b_vram_ms = cyc_to_ms(dcb[BbSections::ObtainVram]);
    // Fold ObtainVram into Buffers and DispatchRecord into RenderPass+Draw/Disp so all 5 sub-rows
    // stay permanently visible without flickering legacy rows.
    h.b_buffers_ms = cyc_to_ms(dcb[BbSections::BindBuffers] + b_vert_idx_cyc) + h.b_vram_ms;
    h.b_tex_desc_ms = cyc_to_ms(dcb[BbSections::BindTextures] + dcb[BbSections::PipelineBind]);
    h.b_motion_hle_ms = cyc_to_ms(dcb[BbSections::Motion] +
                                  dcb[BbSections::Preupload] + dcb[BbSections::DispatchRecord]);
    h.b_state_pass_ms = cyc_to_ms(dcb[BbSections::PrepareRenderState] +
                                  dcb[BbSections::BeginRendering] +
                                  dcb[BbSections::EmitVertexBuffers] +
                                  dcb[BbSections::UpdateDynamicState] + dcb[BbSections::Kick]) +
                        h.b_motion_hle_ms;
    h.b_tasks_ms = std::max(0.0f, cyc_to_ms(dcb[BbSections::Task]) - h.gpu_present_ms);
    h.b_packet_misc_ms =
        std::max(0.0f, h.b_total_ms - (h.b_buffers_ms + h.b_tex_desc_ms + h.b_state_pass_ms +
                                       h.b_tasks_ms));
    h.b_flush_ms = cyc_to_ms(dcb[BbSections::Flush]);
    h.b_busy_pct =
        h.frame_ms > 0.01f ? std::clamp(100.0f * h.b_total_ms / h.frame_ms, 0.0f, 100.0f) : 0.0f;

    // O(1) ~15-second sliding window gameplay average (ignores <300-draw loading/menu screens).
    static HudSlidingWindow win{};
    if (reset_hud_avg.exchange(false, std::memory_order_relaxed)) {
        win.Reset();
    }
    const float gpu_wait_now_ms = h.gpu_present_ms + h.sync_rec_ms + h.host_copy_ms;
    if (h.draws_pf >= 300.0f) {
        const double f_weight = double(df);
        HudSampleSlot s{};
        s.frames = df;
        s.time_s = dt;
        s.present_ms_sum = present_ms_sum_accum;
        s.present_ms_sq_sum = present_ms_sq_sum_accum;
        s.present_ms_count = present_ms_count_accum;
        s.draws = d_draws;
        s.dispatches = d_dispatches;
        s.submits = d_submits;
        s.faults = d_faults;
        s.native_buf_hits = dh;
        s.native_buf_total = dh + dm;
        s.native_tex_hits = dth;
        s.native_tex_total = dth + dtm;
        s.desc_saved = dds;
        s.desc_total = ddt;
        s.idle_a_ms = double(h.idle_a_ms) * f_weight;
        s.gnm_wait_ms = double(h.gnm_wait_ms) * f_weight;
        s.a_pm4_pipe_ms = double(h.a_pm4_pipe_ms) * f_weight;
        s.a_ring_ms = double(h.a_ring_ms) * f_weight;
        s.a_drain_ms = double(h.a_drain_ms) * f_weight;
        s.b_total_ms = double(h.b_total_ms) * f_weight;
        s.b_buffers_ms = double(h.b_buffers_ms) * f_weight;
        s.b_tex_desc_ms = double(h.b_tex_desc_ms) * f_weight;
        s.b_state_pass_ms = double(h.b_state_pass_ms) * f_weight;
        s.b_tasks_ms = double(h.b_tasks_ms) * f_weight;
        s.b_packet_misc_ms = double(h.b_packet_misc_ms) * f_weight;
        s.gpu_wait_ms = double(gpu_wait_now_ms) * f_weight;
        if (d_hw_frames > 0) {
            const double hw_weight = double(d_hw_frames);
            s.hw_gpu_frames = d_hw_frames;
            s.hw_gpu_busy_ms = double(h.hw_gpu_busy_ms) * hw_weight;
            s.hw_gpu_gap_ms = double(h.hw_gpu_gap_ms) * hw_weight;
        }
        win.Push(s);
    }
    present_ms_sum_accum = 0.0;
    present_ms_sq_sum_accum = 0.0;
    present_ms_count_accum = 0;

    if (win.sum.frames > 0 && win.sum.time_s > 0.0) {
        const double inv_af = 1.0 / double(win.sum.frames);
        h.has_avg = true;
        h.window_s = float(win.sum.time_s);
        h.avg_fps = float(double(win.sum.frames) / win.sum.time_s);
        h.avg_frame_ms = float((win.sum.time_s * 1000.0) * inv_af);
        if (win.sum.present_ms_count > 1) {
            const double mean_p = win.sum.present_ms_sum / double(win.sum.present_ms_count);
            const double var_p = std::max(
                0.0, (win.sum.present_ms_sq_sum / double(win.sum.present_ms_count)) - mean_p * mean_p);
            h.avg_jitter_ms = float(std::sqrt(var_p));
        } else {
            h.avg_jitter_ms = h.jitter_ms;
        }
        h.avg_draws_pf = float(double(win.sum.draws) * inv_af);
        h.avg_dispatches_pf = float(double(win.sum.dispatches) * inv_af);
        h.avg_submits_pf = float(double(win.sum.submits) * inv_af);
        h.avg_faults_ps = float(double(win.sum.faults) / win.sum.time_s);
        h.avg_native_buf_pct = win.sum.native_buf_total > 0
                                   ? float(100.0 * double(win.sum.native_buf_hits) /
                                           double(win.sum.native_buf_total))
                                   : h.native_buf_pct;
        h.avg_native_tex_pct = win.sum.native_tex_total > 0
                                   ? float(100.0 * double(win.sum.native_tex_hits) /
                                           double(win.sum.native_tex_total))
                                   : h.native_tex_pct;
        h.avg_desc_saved_pct =
            win.sum.desc_total > 0
                ? float(100.0 * double(win.sum.desc_saved) / double(win.sum.desc_total))
                : h.desc_saved_pct;
        h.avg_idle_a_ms = float(win.sum.idle_a_ms * inv_af);
        h.avg_gnm_wait_ms = float(win.sum.gnm_wait_ms * inv_af);
        h.avg_a_pm4_pipe_ms = float(win.sum.a_pm4_pipe_ms * inv_af);
        h.avg_a_ring_ms = float(win.sum.a_ring_ms * inv_af);
        h.avg_a_drain_ms = float(win.sum.a_drain_ms * inv_af);
        h.avg_a_total_ms = h.avg_a_pm4_pipe_ms + h.avg_a_ring_ms + h.avg_a_drain_ms;
        h.avg_b_total_ms = float(win.sum.b_total_ms * inv_af);
        h.avg_b_buffers_ms = float(win.sum.b_buffers_ms * inv_af);
        h.avg_b_tex_desc_ms = float(win.sum.b_tex_desc_ms * inv_af);
        h.avg_b_state_pass_ms = float(win.sum.b_state_pass_ms * inv_af);
        h.avg_b_tasks_ms = float(win.sum.b_tasks_ms * inv_af);
        h.avg_b_packet_misc_ms = float(win.sum.b_packet_misc_ms * inv_af);
        h.avg_b_busy_pct =
            h.avg_frame_ms > 0.01f
                ? std::clamp(100.0f * h.avg_b_total_ms / h.avg_frame_ms, 0.0f, 100.0f)
                : h.b_busy_pct;
        h.avg_gpu_wait_ms = float(win.sum.gpu_wait_ms * inv_af);
        if (win.sum.hw_gpu_frames > 0) {
            const double inv_hw = 1.0 / double(win.sum.hw_gpu_frames);
            h.avg_hw_gpu_busy_ms = float(win.sum.hw_gpu_busy_ms * inv_hw);
            h.avg_hw_gpu_gap_ms = float(win.sum.hw_gpu_gap_ms * inv_hw);
        } else {
            h.avg_hw_gpu_busy_ms = h.hw_gpu_busy_ms;
            h.avg_hw_gpu_gap_ms = h.hw_gpu_gap_ms;
        }
    } else {
        h.has_avg = false;
        h.window_s = 0.0f;
        h.avg_fps = h.fps;
        h.avg_frame_ms = h.frame_ms;
        h.avg_jitter_ms = h.jitter_ms;
        h.avg_draws_pf = h.draws_pf;
        h.avg_dispatches_pf = h.dispatches_pf;
        h.avg_submits_pf = h.submits_pf;
        h.avg_faults_ps = h.faults_ps;
        h.avg_native_buf_pct = h.native_buf_pct;
        h.avg_native_tex_pct = h.native_tex_pct;
        h.avg_desc_saved_pct = h.desc_saved_pct;
        h.avg_idle_a_ms = h.idle_a_ms;
        h.avg_gnm_wait_ms = h.gnm_wait_ms;
        h.avg_a_pm4_pipe_ms = h.a_pm4_pipe_ms;
        h.avg_a_ring_ms = h.a_ring_ms;
        h.avg_a_drain_ms = h.a_drain_ms;
        h.avg_a_total_ms = h.a_total_ms;
        h.avg_b_total_ms = h.b_total_ms;
        h.avg_b_buffers_ms = h.b_buffers_ms;
        h.avg_b_tex_desc_ms = h.b_tex_desc_ms;
        h.avg_b_state_pass_ms = h.b_state_pass_ms;
        h.avg_b_tasks_ms = h.b_tasks_ms;
        h.avg_b_packet_misc_ms = h.b_packet_misc_ms;
        h.avg_b_busy_pct = h.b_busy_pct;
        h.avg_gpu_wait_ms = gpu_wait_now_ms;
        h.avg_hw_gpu_busy_ms = h.hw_gpu_busy_ms;
        h.avg_hw_gpu_gap_ms = h.hw_gpu_gap_ms;
    }

    // Determine primary bottleneck across parallel stages (and detect active FPS limiter):
    const auto effective_fps_cap = []() noexcept -> int {
        if (const int live = BbFrameRate::Limit(); live > 0) {
            return live;
        }
        static const int env_cap = [] {
            if (const char* lim = std::getenv("BB_FPS_LIMIT"); lim && std::atoi(lim) > 0) {
                return std::atoi(lim);
            }
            if (const char* fps = std::getenv("BB_FPS");
                fps && *fps && std::strcmp(fps, "uncap") != 0 && std::atoi(fps) > 0) {
                return std::atoi(fps);
            }
            return 0;
        }();
        return env_cap;
    };
    const int fps_cap = effective_fps_cap();
    h.fps_cap_active = fps_cap;
    const float cap_budget_ms = fps_cap > 0 ? (1000.0f / float(fps_cap)) : 0.0f;
    const float game_cpu_bound_ms = std::max(0.0f, h.idle_a_ms - h.a_drain_ms);
    const float stage_a_work_ms = h.a_pm4_pipe_ms + h.a_ring_ms;
    const float avg_stage_a_work_ms = h.avg_a_pm4_pipe_ms + h.avg_a_ring_ms;
    const float serial_a_chain_ms = game_cpu_bound_ms + h.gnm_wait_ms + stage_a_work_ms;
    const float stage_b_work_ms = h.b_total_ms;
    const float gpu_sync_ms = h.gpu_present_ms >= 0.50f
                                  ? (h.b_total_ms + h.gpu_present_ms + h.sync_rec_ms)
                                  : gpu_wait_now_ms;
    const float max_active_ms =
        std::max({stage_b_work_ms, stage_a_work_ms, h.hw_gpu_busy_ms, h.gnm_wait_ms});

    if (fps_cap > 0 && h.fps >= float(fps_cap) * 0.92f && max_active_ms < cap_budget_ms * 0.92f) {
        h.bottleneck_kind = 5;
        if (stage_b_work_ms >= stage_a_work_ms && stage_b_work_ms >= h.hw_gpu_busy_ms) {
            h.bottleneck_ms = stage_b_work_ms;
            h.avg_bottleneck_ms = h.avg_b_total_ms;
            h.bottleneck_ru = "Лимит FPS (макс. нагрузка: Stage B)";
            h.bottleneck_en = "FPS Cap Active (Max: Stage B)";
        } else if (h.hw_gpu_busy_ms >= stage_a_work_ms) {
            h.bottleneck_ms = h.hw_gpu_busy_ms;
            h.avg_bottleneck_ms = h.avg_hw_gpu_busy_ms;
            h.bottleneck_ru = "Лимит FPS (макс. нагрузка: GPU Shaders)";
            h.bottleneck_en = "FPS Cap Active (Max: GPU Shaders)";
        } else {
            h.bottleneck_ms = stage_a_work_ms;
            h.avg_bottleneck_ms = avg_stage_a_work_ms;
            h.bottleneck_ru = "Лимит FPS (макс. нагрузка: Stage A)";
            h.bottleneck_en = "FPS Cap Active (Max: Stage A)";
        }
    } else {
        if (h.gnm_wait_ms >= 1.50f && h.b_busy_pct < 78.0f && h.gpu_present_ms < 0.50f) {
            h.bottleneck_kind = 4;
            h.bottleneck_ms = h.gnm_wait_ms;
            h.avg_bottleneck_ms = h.avg_gnm_wait_ms;
            h.bottleneck_ru = "Ожидание GPU кадра (Guest GnmWait)";
            h.bottleneck_en = "CPU<->GPU Frame Sync (Guest GnmWait)";
        } else if (h.gpu_present_ms >= 0.50f && gpu_sync_ms >= stage_b_work_ms) {
            h.bottleneck_kind = 0;
            h.bottleneck_ms = gpu_sync_ms;
            h.avg_bottleneck_ms = h.avg_gpu_wait_ms;
            h.bottleneck_ru = "Видеокарта (GPU Present / Sync)";
            h.bottleneck_en = "GPU Execution (Present / Sync)";
        } else if ((h.b_busy_pct >= 90.0f || stage_b_work_ms >= serial_a_chain_ms) &&
                   stage_b_work_ms >= gpu_sync_ms) {
            h.bottleneck_kind = 3;
            h.bottleneck_ms = stage_b_work_ms;
            h.avg_bottleneck_ms = h.avg_b_total_ms;
            if (h.b_buffers_ms >= h.b_tex_desc_ms && h.b_buffers_ms >= h.b_tasks_ms &&
                h.b_buffers_ms >= h.b_state_pass_ms) {
                h.bottleneck_ru = "Stage B: Буферы (Vtx/Idx/CB)";
                h.bottleneck_en = "Stage B: Buffers (Vtx/Idx/CB)";
            } else if (h.b_tex_desc_ms >= h.b_tasks_ms && h.b_tex_desc_ms >= h.b_state_pass_ms) {
                h.bottleneck_ru = "Stage B: Views + Дескрипторы";
                h.bottleneck_en = "Stage B: Views + Descriptors";
            } else if (h.b_tasks_ms >= h.b_state_pass_ms) {
                h.bottleneck_ru = "Stage B: Задачи / Fences / Copy";
                h.bottleneck_en = "Stage B: Tasks / Fences / Copy";
            } else {
                h.bottleneck_ru = "Stage B: Рендер-пасс + Draw/Disp";
                h.bottleneck_en = "Stage B: RenderPass + Draw/Disp";
            }
        } else if (game_cpu_bound_ms >= stage_a_work_ms && game_cpu_bound_ms >= gpu_sync_ms) {
            h.bottleneck_kind = 1;
            h.bottleneck_ms = game_cpu_bound_ms;
            h.avg_bottleneck_ms = std::max(0.0f, h.avg_idle_a_ms - h.avg_a_drain_ms);
            if (h.gnm_wait_ms > 1.0f) {
                h.bottleneck_ru = "Игра (CPU) + Ожидание Gnm SubmitDone";
                h.bottleneck_en = "Game CPU + Gnm SubmitDone Wait";
            } else {
                h.bottleneck_ru = "Потоки движка игры (Game CPU)";
                h.bottleneck_en = "Game Engine CPU (between submits)";
            }
        } else if (stage_a_work_ms >= gpu_sync_ms) {
            h.bottleneck_kind = 2;
            h.bottleneck_ms = stage_a_work_ms;
            h.avg_bottleneck_ms = avg_stage_a_work_ms;
            if (h.a_ring_ms > h.a_pm4_pipe_ms) {
                h.bottleneck_ru = "Stage A: Поток констант (ConstRing)";
                h.bottleneck_en = "Stage A: Constant Stream (ConstRing)";
            } else {
                h.bottleneck_ru = "Stage A: DirectVk RHI + FastPSO";
                h.bottleneck_en = "Stage A: DirectVk RHI + FastPSO";
            }
        } else {
            h.bottleneck_kind = 0;
            h.bottleneck_ms = gpu_sync_ms;
            h.avg_bottleneck_ms = h.avg_gpu_wait_ms;
            h.bottleneck_ru = "Видеокарта (GPU Present / Sync)";
            h.bottleneck_en = "GPU Execution (Present / Sync)";
        }
    }

    last_time = now;
    last_tsc = tsc;
    last_frames = cur_frames;
    last_draws = cur_draws;
    last_dispatches = cur_dispatches;
    last_submits = cur_submits;
    last_idle_ns = cur_idle_ns;
    last_gnm_wait_ns = cur_gnm_wait_ns;
    last_present_ns = cur_present_ns;
    last_sync_rec_ns = cur_sync_rec_ns;
    last_host_copy_ns = cur_host_copy_ns;
    last_pipe_drains = cur_pipe_drains;
    last_pipe_drain_cyc = cur_pipe_drain_cyc;
    last_pipe_busy_cyc = cur_pipe_busy_cyc;
    last_faults = cur_faults;
    last_eop = cur_eop;
    last_native_hits = cur_native_hits;
    last_native_misses = cur_native_misses;
    last_native_tex_hits = cur_native_tex_hits;
    last_native_tex_misses = cur_native_tex_misses;
    last_desc_saved = cur_desc_saved;
    last_desc_total = cur_desc_total;
    if (d_hw_frames > 0) {
        last_hw_busy_ns = cur_hw_busy_ns;
        last_hw_gap_ns = cur_hw_gap_ns;
        last_hw_frames = cur_hw_frames;
    }

    if (std::chrono::duration<double>(now - last_file_dump).count() >= 1.0) {
        last_file_dump = now;
        if (FILE* f = std::fopen("/tmp/bb_frame_hud.txt", "w")) {
            const auto& s = BbSettings::Get();
            const auto out = BbSettings::CurrentOutput();
            const auto shd = Vulkan::DirectShaders::ReadTelemetry();
            std::fprintf(f,
                         "FPS: %.1f (%.2f ms/f) | Avg15s: %.1f FPS (%.2f ms/f, Jit %.2f ms) | Scene: %dx%d -> %dx%d | Upscaler: %s (%s)%s\n"
                         "Counts/f: Draws %.0f (Avg %.0f) | Disp %.0f (Avg %.0f) | Subm %.1f (Avg %.1f) | PipeDrains %.1f | Faults/s %.1f | NativeVkBuf %.1f%% | NativeVkTex %.1f%% | DescSaved %.1f%% | DontCare %llu/%llu\n"
                         "DirectShaders: reg %llu | manifest %llu/%u | stageA_hits %llu | worker_hits %llu\n"
                         "1. Game CPU / Gnm: IdleA %.2f ms (Avg %.2f) | GnmWait %.2f ms (Avg %.2f)\n"
                         "2. Stage A (GpuComm): Total %.2f ms (Avg %.2f) | PM4+FastPSO %.2f ms (Avg %.2f) | ConstRing %.2f ms (Avg %.2f) | WaitDrainB %.2f ms\n"
                         "3. Stage B (DrawRec): Total %.2f ms (Avg %.2f) | Buffers %.2f ms (Avg %.2f) | Tex+Desc %.2f ms (Avg %.2f) | State+Draw+Disp %.2f ms (Avg %.2f) | Tasks %.2f ms (Avg %.2f) | Packet+Regs %.2f ms (Avg %.2f)\n"
                         "4. GPU / Sync: Present+Sync %.2f ms (Avg %.2f) | HwGpuBusy %.2f ms (Avg %.2f) | HwGpuGap %.2f ms (Avg %.2f)\n"
                         "BOTTLENECK: %s (Now %.2f ms / Avg %.2f ms)\n",
                         h.fps, h.frame_ms, h.avg_fps, h.avg_frame_ms, h.avg_jitter_ms,
                         s.active_render_width.load(), s.active_render_height.load(),
                         out.width, out.height,
                         BbSettings::UpscalerName(s.upscaler),
                         BbSettings::PresetName(BbSettings::RenderPreset()),
                         BbSettings::ResolutionNeedsRestart() ? " [NEEDS RESTART]" : "",
                         h.draws_pf, h.avg_draws_pf, h.dispatches_pf, h.avg_dispatches_pf,
                         h.submits_pf, h.avg_submits_pf, h.drains_pf,
                         h.faults_ps, h.native_buf_pct, h.native_tex_pct,
                         h.desc_saved_pct,
                         static_cast<unsigned long long>(BbStats::dont_care_passes.load(std::memory_order_relaxed)),
                         static_cast<unsigned long long>(BbStats::total_render_passes.load(std::memory_order_relaxed)),
                         static_cast<unsigned long long>(shd.registered_binaries),
                         static_cast<unsigned long long>(shd.manifest_binaries_bound),
                         shd.manifest_total,
                         static_cast<unsigned long long>(shd.stage_a_direct_hits),
                         static_cast<unsigned long long>(shd.worker_direct_hits),
                         h.idle_a_ms, h.avg_idle_a_ms, h.gnm_wait_ms, h.avg_gnm_wait_ms,
                         h.a_total_ms, h.avg_a_total_ms, h.a_pm4_pipe_ms, h.avg_a_pm4_pipe_ms,
                         h.a_ring_ms, h.avg_a_ring_ms, h.a_drain_ms,
                         h.b_total_ms, h.avg_b_total_ms, h.b_buffers_ms, h.avg_b_buffers_ms,
                         h.b_tex_desc_ms, h.avg_b_tex_desc_ms, h.b_state_pass_ms, h.avg_b_state_pass_ms,
                         h.b_tasks_ms, h.avg_b_tasks_ms, h.b_packet_misc_ms, h.avg_b_packet_misc_ms,
                         gpu_wait_now_ms, h.avg_gpu_wait_ms,
                         h.hw_gpu_busy_ms, h.avg_hw_gpu_busy_ms, h.hw_gpu_gap_ms, h.avg_hw_gpu_gap_ms,
                         h.bottleneck_en, h.bottleneck_ms, h.avg_bottleneck_ms);
            std::fclose(f);
        }
    }
}

void FpsCounter() {
    static HudBreakdown h{};
    UpdateHudBreakdown(h);

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float pad = 12.0f * base_scale;
    ImGui::SetNextWindowPos(
        ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - pad, viewport->WorkPos.y + pad),
        ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.78f);
    ImGui::Begin("##fps", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav |
                     ImGuiWindowFlags_NoFocusOnAppearing);
    const auto& s = BbSettings::Get();
    const auto out = BbSettings::CurrentOutput();
    const int rw = s.active_render_width.load(std::memory_order_relaxed);
    const int rh = s.active_render_height.load(std::memory_order_relaxed);
    const char* upscaler_str =
        s.upscaler == BbSettings::UpscalerFsr3     ? "FSR 3.1"
        : s.upscaler == BbSettings::UpscalerFsr4   ? "FSR 4"
        : s.upscaler == BbSettings::UpscalerFsr411 ? "FSR 4.1.1"
        : s.upscaler == BbSettings::UpscalerTaa    ? "TAA"
        : s.upscaler == BbSettings::UpscalerDlss
            ? (BbSettings::RenderPreset() == BbSettings::NativeAA
                   ? "DLAA 4"
                   : (s.dlss_preset.load(std::memory_order_relaxed) == 13   ? "DLSS 4.5 M"
                      : s.dlss_preset.load(std::memory_order_relaxed) == 12 ? "DLSS 4.5 L"
                      : s.dlss_preset.load(std::memory_order_relaxed) != 0  ? "DLSS 4 K"
                                                                            : "DLSS Auto"))
            : (s.effects[4].load(std::memory_order_relaxed) ? "FXAA" : "Off");

    const float now_fps = frame_ms_avg > 0.0f ? 1000.0f / frame_ms_avg : h.fps;
    const float now_ms = frame_ms_avg > 0.0f ? frame_ms_avg : h.frame_ms;
    if (h.has_avg) {
        ImGui::TextColored(
            ImVec4(0.95f, 0.92f, 0.78f, 1.0f),
            BbSettings::MenuText("%.0f FPS (Avg %.0f)  %.2f ms (Avg %.2f)  [%s]",
                                 "%.0f FPS (Avg %.0f)  %.2f мс (Avg %.2f)  [%s]"),
            now_fps, h.avg_fps, now_ms, h.avg_frame_ms, upscaler_str);
    } else {
        ImGui::TextColored(ImVec4(0.95f, 0.92f, 0.78f, 1.0f),
                           BbSettings::MenuText("%.0f FPS  %.2f ms  [%s]",
                                                "%.0f FPS  %.2f мс  [%s]"),
                           now_fps, now_ms, upscaler_str);
    }
    const int fps_cap = h.fps_cap_active;
    char cap_buf[32];
    if (fps_cap > 0) {
        std::snprintf(cap_buf, sizeof(cap_buf), "Cap %d", fps_cap);
    } else {
        std::snprintf(cap_buf, sizeof(cap_buf), "Uncapped");
    }
    ImGui::TextDisabled(
        BbSettings::MenuText("Scene: %dx%d -> %dx%d | %s | Jit: %.2f ms (Avg %.2f)%s",
                             "Сцена: %dx%d -> %dx%d | %s | Джиттер: %.2f мс (Avg %.2f)%s"),
        rw, rh, out.width, out.height, cap_buf, h.jitter_ms, h.avg_jitter_ms,
        BbSettings::ResolutionNeedsRestart()
            ? BbSettings::MenuText(" (restart!)", " (перезапуск!)")
            : "");
    ImGui::TextDisabled(
        BbSettings::MenuText(
            "Draws: %.0f (%.0f) | Disp: %.0f (%.0f) | Subm: %.1f | B busy: %.0f%% (%.0f%%)",
            "Draws: %.0f (%.0f) | Disp: %.0f (%.0f) | Subm: %.1f | B занят: %.0f%% (%.0f%%)"),
        h.draws_pf, h.avg_draws_pf, h.dispatches_pf, h.avg_dispatches_pf, h.submits_pf,
        h.b_busy_pct, h.avg_b_busy_pct);
    const ImVec4 health_col = (h.faults_ps < 10.0f && h.eop_pf < 5.0f)
                                  ? ImVec4(0.55f, 0.88f, 0.62f, 1.0f)
                                  : ImVec4(1.0f, 0.72f, 0.38f, 1.0f);
    ImGui::TextColored(
        health_col,
        BbSettings::MenuText(
            "Faults: %.0f/s (%.0f) | NatBuf: %.0f%% (%.0f%%) | NatTex: %.0f%% | DscSav: %.0f%% (%.0f%%)",
            "Фолты: %.0f/с (%.0f) | NatBuf: %.0f%% (%.0f%%) | NatTex: %.0f%% | DscSav: %.0f%% (%.0f%%)"),
        h.faults_ps, h.avg_faults_ps, h.native_buf_pct, h.avg_native_buf_pct, h.native_tex_pct,
        h.desc_saved_pct, h.avg_desc_saved_pct);

    ImGui::Separator();
    if (ImGui::BeginTable("##hud_breakdown", 3,
                          ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX)) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextDisabled("%s", BbSettings::MenuText("Pipeline Stage (C++20 + DirectVk)",
                                                       "Этап конвейера (C++20 + DirectVk)"));
        ImGui::TableSetColumnIndex(1);
        ImGui::TextDisabled("%s", BbSettings::MenuText("   Now", "Сейчас"));
        ImGui::TableSetColumnIndex(2);
        ImGui::TextDisabled("%s", BbSettings::MenuText("Avg 15s", "Avg 15с"));

        const auto row = [&](const char* label_en, const char* label_ru, float ms, float avg_ms,
                             bool highlight = false) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            const ImVec4 col = highlight     ? ImVec4(1.0f, 0.82f, 0.35f, 1.0f)
                               : ms >= 1.50f ? ImVec4(1.0f, 0.60f, 0.45f, 1.0f)
                               : ms >= 0.75f ? ImVec4(0.95f, 0.92f, 0.75f, 1.0f)
                                             : ImVec4(0.75f, 0.78f, 0.82f, 1.0f);
            ImGui::TextColored(col, "%s", BbSettings::MenuText(label_en, label_ru));
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(col, BbSettings::MenuText("%5.2f ms", "%5.2f мс"), ms);
            ImGui::TableSetColumnIndex(2);
            if (h.has_avg) {
                ImGui::TextDisabled("%5.2f", avg_ms);
            } else {
                ImGui::TextDisabled("  -- ");
            }
        };

        // 1. Game CPU / Gnm (2 fixed rows)
        row("1. Game CPU (Idle A)", "1. Игра CPU (ожид. батчей)", h.idle_a_ms, h.avg_idle_a_ms,
            h.bottleneck_kind == 1);
        row("  Guest GnmWait (Sync)", "  Блок GnmWait (CPU<->GPU)", h.gnm_wait_ms,
            h.avg_gnm_wait_ms, h.bottleneck_kind == 4);

        // 2. Stage A (bb:GpuComm) (3 fixed rows: Total + 2 sub-rows)
        row("2. Stage A: Total (GpuComm)", "2. Stage A: Всего (GpuComm)", h.a_total_ms,
            h.avg_a_total_ms, h.bottleneck_kind == 2);
        row("  DirectVk RHI + FastPSO", "  DirectVk RHI + FastPSO", h.a_pm4_pipe_ms,
            h.avg_a_pm4_pipe_ms, h.bottleneck_kind == 2 && h.a_pm4_pipe_ms >= h.a_ring_ms);
        row("  ConstRing + Drain", "  Поток констант + Drain", h.a_ring_ms + h.a_drain_ms,
            h.avg_a_ring_ms + h.avg_a_drain_ms,
            h.bottleneck_kind == 2 && h.a_ring_ms > h.a_pm4_pipe_ms);

        // 3. Stage B (bb:DrawRec) (6 fixed rows: Total + 5 sub-rows)
        row("3. Stage B: Vulkan Record", "3. Stage B: Запись Vulkan", h.b_total_ms,
            h.avg_b_total_ms, h.bottleneck_kind == 3);
        row("  Buffers (Vtx/Idx/CB)", "  Буферы (Vtx/Idx/Const)", h.b_buffers_ms,
            h.avg_b_buffers_ms);
        row("  Views + Descriptors", "  Views + Дескрипторы", h.b_tex_desc_ms,
            h.avg_b_tex_desc_ms);
        row("  RenderPass + Draw/Disp", "  Рендер-пасс + Draw/Disp", h.b_state_pass_ms,
            h.avg_b_state_pass_ms);
        row("  Tasks / Fences / Copy", "  Задачи / Fences / Copy", h.b_tasks_ms,
            h.avg_b_tasks_ms);
        row("  Packet + Regs + CmdPush", "  Пакет + Регистры + Стрим", h.b_packet_misc_ms,
            h.avg_b_packet_misc_ms);

        // 4. GPU / Sync + Hardware GPU Timestamps (3 fixed rows)
        row("4. GPU Present / Sync", "4. Ожидание GPU / Синхр.",
            h.gpu_present_ms + h.sync_rec_ms + h.host_copy_ms, h.avg_gpu_wait_ms,
            h.bottleneck_kind == 0);
        row("  HW GPU Busy (Shaders)", "  Чистая работа GPU (Шейдеры)", h.hw_gpu_busy_ms,
            h.avg_hw_gpu_busy_ms,
            h.bottleneck_kind == 0 && h.hw_gpu_busy_ms >= h.b_total_ms);
        row("  HW GPU Mid-Frame Gap", "  Паузы GPU между батчами", h.hw_gpu_gap_ms,
            h.avg_hw_gpu_gap_ms);

        ImGui::EndTable();
    }
    ImGui::Separator();
    const ImVec4 limit_col = h.bottleneck_kind == 5 ? ImVec4(0.55f, 0.90f, 0.68f, 1.0f)
                                                    : ImVec4(1.0f, 0.78f, 0.28f, 1.0f);
    ImGui::TextColored(
        limit_col,
        BbSettings::MenuText("LIMIT: %s (%.2f ms | Avg %.2f)", "ЛИМИТ: %s (%.2f мс | Avg %.2f)"),
        BbSettings::MenuText(h.bottleneck_en, h.bottleneck_ru), h.bottleneck_ms,
        h.avg_bottleneck_ms);
    ImGui::End();
}

void TextPrompt() {
    std::string title, text;
    {
        std::scoped_lock lock{prompt_mutex};
        title = prompt_title;
        text = prompt_text;
    }
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
                                   viewport->WorkPos.y + viewport->WorkSize.y * 0.5f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowBgAlpha(0.9f);
    ImGui::Begin("##textprompt", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav |
                     ImGuiWindowFlags_NoFocusOnAppearing);
    ImGui::TextUnformatted(title.c_str());
    ImGui::Separator();
    ImGui::Text("%s_", text.c_str());
    ImGui::Separator();
    ImGui::TextUnformatted("Keyboard: type, Backspace = delete, Enter = OK, Esc = cancel");
    ImGui::End();
}

/// BB_MENU_KEYS_FILE=<file> (scripted tests): tokens toggle up down left right enter back l1 r1,
/// consumed when the file appears (it is removed), one key press per frame.
void ScriptedKeys() {
    static const char* path = std::getenv("BB_MENU_KEYS_FILE");
    static std::chrono::steady_clock::time_point last_check{};
    static std::vector<std::string> queue;
    static bool release = false;
    static ImGuiKey held = ImGuiKey_None;
    if (!path) {
        return;
    }
    ImGuiIO& io = ImGui::GetIO();
    if (release) {
        io.AddKeyEvent(held, false);
        release = false;
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    if (queue.empty() && now - last_check > std::chrono::milliseconds(100)) {
        last_check = now;
        if (FILE* f = std::fopen(path, "r")) {
            char token[32];
            while (std::fscanf(f, "%31s", token) == 1) {
                queue.emplace_back(token);
            }
            std::fclose(f);
            std::remove(path);
        }
    }
    if (queue.empty()) {
        return;
    }
    const std::string token = queue.front();
    queue.erase(queue.begin());
    if (token == "toggle") {
        SetOpen(!menu_open);
        return;
    }
    held = token == "up" ? ImGuiKey_GamepadDpadUp : token == "down" ? ImGuiKey_GamepadDpadDown
         : token == "left" ? ImGuiKey_GamepadDpadLeft : token == "right" ? ImGuiKey_GamepadDpadRight
         : token == "enter" ? ImGuiKey_GamepadFaceDown : token == "l1" ? ImGuiKey_GamepadL1
         : token == "r1" ? ImGuiKey_GamepadR1 : token == "kdown" ? ImGuiKey_DownArrow
         : token == "kup" ? ImGuiKey_UpArrow : ImGuiKey_None;
    if (token == "back") {
        SetOpen(false);
        return;
    }
    if (held != ImGuiKey_None) {
        io.AddKeyEvent(held, true);
        release = true;
    }
}

} // namespace

void SetTextPrompt(bool active, const std::string& prompt, const std::string& text) {
    {
        std::scoped_lock lock{prompt_mutex};
        prompt_title = prompt;
        prompt_text = text;
    }
    prompt_active = active;
}

void Init(const Vulkan::Instance& instance, vk::Format format, u32 image_count) {
    std::scoped_lock lock{imgui_mutex};
    if (initialized) {
        return;
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // window positions are not kept
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    io.BackendPlatformName = "bbport";

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.Colors[ImGuiCol_WindowBg].w = 0.92f;

    ImFontConfig font_config;
    font_config.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(bb_font_ttf),
                                   int(bb_font_ttf_end - bb_font_ttf), 18.0f, &font_config);

    const vk::Instance vk_instance = instance.GetInstance();
    ImGui_ImplVulkan_LoadFunctions(
        instance.ApiVersion(),
        [](const char* name, void* user) {
            return VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr(
                *static_cast<const vk::Instance*>(user), name);
        },
        const_cast<vk::Instance*>(&vk_instance));

    const VkFormat color_format = static_cast<VkFormat>(format);
    ImGui_ImplVulkan_InitInfo info{};
    info.ApiVersion = instance.ApiVersion();
    info.Instance = vk_instance;
    info.PhysicalDevice = instance.GetPhysicalDevice();
    info.Device = instance.GetDevice();
    info.QueueFamily = instance.GetGraphicsQueueFamilyIndex();
    info.Queue = instance.GetGraphicsQueue();
    info.DescriptorPoolSize = 16;
    info.MinImageCount = std::max(image_count, 2u);
    info.ImageCount = std::max(image_count, 2u);
    info.UseDynamicRendering = true;
    info.PipelineInfoMain.PipelineRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &color_format,
    };
    if (!ImGui_ImplVulkan_Init(&info)) {
        std::printf("Overlay: ImGui Vulkan backend init failed\n");
        ImGui::DestroyContext();
        return;
    }
    initialized = true;
    std::printf("Overlay: menu ready (Insert or L3+R3)\n");
}

void UpdateTextInput(SDL_Window* window) {
    bool want = false;
    {
        std::scoped_lock lock{imgui_mutex};
        want = initialized && menu_open && ImGui::GetIO().WantTextInput;
    }
    if (want != SDL_TextInputActive(window)) {
        if (want) {
            SDL_StartTextInput(window);
        } else {
            SDL_StopTextInput(window);
        }
    }
}

bool HandleEvent(const SDL_Event& event) {
    std::scoped_lock lock{imgui_mutex};
    if (!initialized) {
        return false;
    }
    ImGuiIO& io = ImGui::GetIO();
    const bool is_open = menu_open;
    switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
        const bool down = event.type == SDL_EVENT_KEY_DOWN;
        if (is_open && rebinding_action >= 0) {
            return true;
        }
        if (down && !event.key.repeat &&
            (event.key.key == SDLK_INSERT || (is_open && event.key.key == SDLK_ESCAPE))) {
            SetOpen(event.key.key == SDLK_INSERT ? !is_open : false);
            return true;
        }
        if (!is_open) {
            return false;
        }
        io.AddKeyEvent(ImGuiMod_Ctrl, (event.key.mod & SDL_KMOD_CTRL) != 0);
        io.AddKeyEvent(ImGuiMod_Shift, (event.key.mod & SDL_KMOD_SHIFT) != 0);
        io.AddKeyEvent(ImGuiMod_Alt, (event.key.mod & SDL_KMOD_ALT) != 0);
        if (const ImGuiKey key = KeyFromSdl(event.key.key); key != ImGuiKey_None) {
            io.AddKeyEvent(key, down);
        }
        return true;
    }
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP: {
        const bool down = event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN;
        const u8 button = event.gbutton.button;
        if (button == SDL_GAMEPAD_BUTTON_LEFT_STICK) {
            l3_down = down;
        } else if (button == SDL_GAMEPAD_BUTTON_RIGHT_STICK) {
            r3_down = down;
        }
        if (down && l3_down && r3_down) {
            SetOpen(!is_open);
            return true;
        }
        if (!is_open) {
            return false;
        }
        if (const ImGuiKey key = KeyFromGamepad(button); key != ImGuiKey_None) {
            io.AddKeyEvent(key, down);
        }
        return true;
    }
    case SDL_EVENT_TEXT_INPUT: {
        // Typed characters (Ctrl+click on a slider, a text field): key events alone erase but
        // do not type. SDL sends them while text input is on (UpdateTextInput).
        if (!is_open) {
            return false;
        }
        io.AddInputCharactersUTF8(event.text.text);
        return true;
    }
    case SDL_EVENT_MOUSE_MOTION: {
        if (!is_open) {
            return false;
        }
        const float density = PixelDensity(event.motion.windowID);
        io.AddMousePosEvent(event.motion.x * density, event.motion.y * density);
        return true;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        if (!is_open) {
            return false;
        }
        if (rebinding_action >= 0) {
            if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
                io.AddMouseButtonEvent(0, false);
            }
            return true;
        }
        const int button = event.button.button == SDL_BUTTON_LEFT    ? 0
                           : event.button.button == SDL_BUTTON_RIGHT  ? 1
                           : event.button.button == SDL_BUTTON_MIDDLE ? 2
                                                                      : -1;
        if (button >= 0) {
            io.AddMouseButtonEvent(button, event.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        }
        return true;
    }
    case SDL_EVENT_MOUSE_WHEEL:
        if (!is_open) {
            return false;
        }
        io.AddMouseWheelEvent(event.wheel.x, event.wheel.y);
        return true;
    default:
        return false;
    }
}

void SetBanner(const char* text) {
    {
        std::scoped_lock lock{banner_mutex};
        banner_text = text ? text : "";
    }
    banner_active = text != nullptr && text[0] != 0;
}

bool Visible() {
    return initialized && (menu_open || prompt_active || banner_active ||
                           BbSettings::Get().show_fps);
}

bool MenuOpen() {
    return menu_open;
}

bool CapturesInput() {
    // The text dialog too: keys typed into it (Backspace is the touchpad) stay out of the game.
    return menu_open || prompt_active;
}

void Render(vk::CommandBuffer cmdbuf, vk::ImageView view, vk::Extent2D extent) {

    // Present interval for the FPS readout (measured also while nothing is drawn).
    const auto now = std::chrono::steady_clock::now();
    const float ms = std::chrono::duration<float, std::milli>(now - last_present).count();
    last_present = now;
    if (ms > 0.0f && ms < 1000.0f) {
        frame_ms_avg = frame_ms_avg == 0.0f ? ms : frame_ms_avg * 0.95f + ms * 0.05f;
        present_ms_sum_accum += double(ms);
        present_ms_sq_sum_accum += double(ms) * double(ms);
        ++present_ms_count_accum;
    }
    // BB_FPS_PRINT=1: the present rate every 5 s on stdout, without the cost of BB_FRAME_STATS
    // (its timers are in the draw path): to measure as players run the game.
    static const bool fps_print = [] {
        const char* env = std::getenv("BB_FPS_PRINT");
        return env && env[0] == '1';
    }();
    if (fps_print) {
        static auto window_start = now;
        static std::uint64_t presents = 0;
        ++presents;
        if (const double s = std::chrono::duration<double>(now - window_start).count(); s >= 5.0) {
            std::printf("Present rate: %.1f FPS over %.1f s\n", presents / s, s);
            std::fflush(stdout);
            window_start = now;
            presents = 0;
        }
    }
    if (std::getenv("BB_MENU_KEYS_FILE") && initialized) {
        std::scoped_lock lock{imgui_mutex};
        ScriptedKeys();
    }
    if (!Visible()) {
        return;
    }
    std::scoped_lock lock{imgui_mutex};
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(float(extent.width), float(extent.height));
    io.DeltaTime = ms > 0.0f && ms < 1000.0f ? ms / 1000.0f : 1.0f / 60.0f;
    // UI scale follows the display height (1080p = 1).
    const float scale = std::max(float(extent.height) / 1080.0f, 0.75f);
    if (std::abs(scale - base_scale) > 0.01f) {
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(scale / base_scale);
        style.FontScaleMain = scale;
        base_scale = scale;
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
    if (menu_open) {
        Menu();
    }
    if (BbSettings::Get().show_fps && !menu_open) {
        FpsCounter();
    }
    if (prompt_active && !menu_open) {
        TextPrompt();
    }
    if (banner_active) {
        std::string text;
        {
            std::scoped_lock banner_lock{banner_mutex};
            text = banner_text;
        }
        const ImVec2 size = ImGui::CalcTextSize(text.c_str());
        auto* draw = ImGui::GetForegroundDrawList();
        const ImVec2 at{(io.DisplaySize.x - size.x) * 0.5f, io.DisplaySize.y * 0.82f};
        draw->AddRectFilled({at.x - 24.0f, at.y - 12.0f}, {at.x + size.x + 24.0f, at.y + size.y + 12.0f},
                            IM_COL32(0, 0, 0, 170), 6.0f);
        draw->AddText(at, IM_COL32(230, 220, 200, 255), text.c_str());
    }
    ImGui::Render();

    const vk::RenderingAttachmentInfo attachment{
        .imageView = view,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eStore,
    };
    cmdbuf.beginRendering(vk::RenderingInfo{
        .renderArea = {{0, 0}, extent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachment,
    });
    {
        // Font atlas uploads submit to the graphics queue themselves.
        std::scoped_lock submit_lock{Vulkan::Scheduler::submit_mutex};
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdbuf);
    }
    cmdbuf.endRendering();
}

} // namespace BbOverlay

extern "C" void bbgpu_overlay_banner(const char* text) {
    BbOverlay::SetBanner(text);
}
