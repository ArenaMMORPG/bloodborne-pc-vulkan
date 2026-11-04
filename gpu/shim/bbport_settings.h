// SPDX-License-Identifier: GPL-2.0-or-later
// bbport: user settings changed at run time from the in-game menu (bbport_overlay.h) and kept
// in bbport.ini (BB_CONFIG overrides the path). Environment variables override the file at
// start. Readers load the atomics every frame; writers are the menu and Load().

#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <string>

namespace BbSettings {

enum Upscaler : int { UpscalerOff = 0, UpscalerFsr3 = 1, UpscalerFsr4 = 2, UpscalerFsr411 = 3,
                      UpscalerTaa = 4, UpscalerDlss = 5, UpscalerCount };
enum AaMode : int { AaOff = 0, AaFxaa = 1, AaTaa = 2, AaDlaa = 3, AaCount };
/// FSR 4 v07 or FSR 4.1.1: the same inputs, settings and placement in the frame.
inline bool IsFsr4(int upscaler) {
    return upscaler == UpscalerFsr4 || upscaler == UpscalerFsr411;
}
enum Preset : int { NativeAA = 0, Quality, Balanced, Performance, UltraPerformance, PresetCount };
enum DebugView : int { DebugNone = 0, DebugReactive = 1, DebugMotion = 2, DebugViewCount };
enum class MenuLanguage { English, Russian };

/// Game effects switched by the community patches at start (patches.py EFFECTS): ini key,
/// menu label, default (the game's own behaviour).
struct Effect {
    const char* key;
    const char* label;
    const char* label_ru;
    bool default_on;
};
inline constexpr Effect Effects[] = {
    {"effect_chromatic_aberration", "Chromatic aberration", "Хроматическая аберрация", true},
    {"effect_dof", "Depth of field (DoF)", "Глубина резкости (DoF)", true},
    {"effect_motion_blur", "Motion blur", "Размытие в движении", true},
    {"effect_ssao", "Ambient occlusion (SSAO)", "Затенение SSAO", true},
    {"effect_game_aa", "Game's own anti-aliasing", "Собственное сглаживание игры", true},
    {"effect_dynamic_shadows", "Shadows from dynamic lights", "Тени от динамических источников", true},
    {"effect_ssr", "Screen-space reflections (not in original game)", "Отражения SSR (не было в игре)", false},
    {"effect_bloom", "Bloom", "Свечение (Bloom)", true},
    {"skip_intro", "Skip startup intros", "Пропуск заставок при запуске", true},
    {"debug_camera", "Free camera (Cross + L3)", "Свободная камера (Cross + L3)", false},
    {"debug_menu", "Debug menu (requires font files)", "Debug menu (нужны файлы шрифтов)", false},
};
inline constexpr int EffectCount = int(sizeof(Effects) / sizeof(Effects[0]));
/// All 8 graphics effects (0..7, including SSR) are switched live in-game without a restart.
inline constexpr bool IsHotEffect(int effect) {
    return effect >= 0 && effect < 8;
}

/// Configurable keyboard & mouse bindings (stored in bbport.ini as key.<name>=..., reloaded live
/// in src/runtime_pad.c without restarting).
enum KeyAction : int {
    KeyAttackR1 = 0,
    KeyStrongR2,
    KeyFirearmL2,
    KeyTransformL1,
    KeyDodge,
    KeySprint,
    KeyHealTriangle,
    KeyItemSquare,
    KeyInteractCross,
    KeyLockOnR3,
    KeySwitchRight,
    KeySwitchLeft,
    KeyGestures,
    KeyActionCount
};

struct KeyActionDef {
    const char* ini_key;
    const char* name_en;
    const char* name_ru;
    const char* help_en;
    const char* help_ru;
    const char* default_val;
};

const KeyActionDef& GetKeyActionDef(int action);
std::string GetKeyBinding(int action);
void SetKeyBinding(int action, const std::string& value);
std::string FormatKeyBinding(const std::string& raw, bool russian = false);
void ResetKeyBindingsToDefault();
/// Output aspect ratios, and the output resolutions of each: the upscaler's output and the UI's
/// targets, changed live. The game keeps its 1920x1080 frame; for another aspect ratio its camera,
/// HUD markers and UI placement follow the output at run time (bbport_guest_hooks, the UI
/// composition). Auto: the group of the monitor the game is on.
enum Aspect : int { AspectAuto = 0, Aspect4x3, Aspect16x10, Aspect16x9, Aspect21x9, Aspect32x9, AspectCount };
struct OutputSize {
    int width = 1920, height = 1080;
    bool operator==(const OutputSize&) const = default;
};
inline constexpr OutputSize OutputDefault{1920, 1080}; ///< the game's own size
constexpr std::uint32_t PackSize(OutputSize s) {
    return std::uint32_t(s.width) | std::uint32_t(s.height) << 16;
}
constexpr OutputSize UnpackSize(std::uint32_t v) {
    return {int(v & 0xffff), int(v >> 16)};
}
/// An aspect ratio's outputs, smallest first.
struct OutputList {
    std::array<OutputSize, 12> sizes{};
    int count = 0;
    const OutputSize* begin() const { return sizes.data(); }
    const OutputSize* end() const { return sizes.data() + count; }
    int IndexOf(OutputSize size) const; ///< -1: not in the list
};
/// bbport.ini value ("auto", "4:3", "16:10", "16:9", "21:9", "32:9").
const char* AspectKey(int aspect);
/// The monitor the game's window is on (window.cpp): Auto follows it. 0x0 until known.
void SetMonitor(int width, int height);
OutputSize Monitor();
/// Auto resolved to the group nearest the monitor's aspect ratio (16:9 until it is known).
int ResolvedAspect(int aspect);
/// The output resolutions of an aspect ratio (Auto: the monitor's group and its own size).
OutputList Outputs(int aspect);
/// The output of `aspect` nearest `size` in height: another aspect ratio keeps the height.
OutputSize NearestOutput(int aspect, OutputSize size);
/// The output in use: the chosen resolution in the chosen aspect ratio.
OutputSize CurrentOutput();
/// "2560x1080".
std::string SizeText(OutputSize size);

struct Values {
    std::atomic<MenuLanguage> menu_language{MenuLanguage::English};
    /// Chosen in the menu (or already in bbport.ini): only then is it saved, so the language
    /// the menu merely started with keeps following the launcher's (#134).
    std::atomic<bool> menu_language_chosen{false};
    std::atomic<int> upscaler{UpscalerFsr3};
    std::atomic<int> aa_mode{AaFxaa};
    std::atomic<bool> aa_mode_chosen{false};
    std::atomic<int> preset{NativeAA};
    std::atomic<bool> sharpen{true};
    std::atomic<float> sharpness{0.3f};
    std::atomic<bool> jitter{true};
    std::atomic<bool> reactive{false};
    std::atomic<bool> object_motion{true};
    std::atomic<float> reactive_scale{1.0f};
    std::atomic<float> reactive_threshold{0.2f};
    std::atomic<float> reactive_max{0.9f};
    std::atomic<int> debug_view{DebugNone};
    std::atomic<bool> show_fps{false};
    /// The settings menu's position (fraction of the screen), -1 until it is moved.
    std::atomic<float> menu_x{-1.0f}, menu_y{-1.0f};
    // FSR 4 checks (menu): the provider's auto exposure, the jitter sign it is given.
    std::atomic<bool> fsr4_auto_exposure{true};
    std::atomic<bool> fsr4_invert_jitter{false};
    std::atomic<int> active_render_width{1920}, active_render_height{1080};
    /// Applied at start (patches.py); the menu shows when a restart is needed.
    std::atomic<bool> effects[EffectCount]{};
    std::atomic<int> model_lod{0}; ///< -2 highest .. 2 lowest, 0 the game's
    std::atomic<int> aspect{AspectAuto};
    /// The chosen output resolution (PackSize; CurrentOutput fits it to the aspect ratio).
    std::atomic<std::uint32_t> output_res{PackSize(OutputDefault)};
    /// Live resolution and preset changes (run.sh): 0 off by default (startup patch, fastest
    /// on the Steam Deck and older GPUs), -1 auto (strong discrete GPUs), 1 on. On restart.
    std::atomic<int> live_resolution{0};
    /// FSR 3 frame generation (upscale/vk_frame_generation): a frame interpolated between each
    /// two of the game's, shown between them (needs the upscaler's scaled path). Live.
    std::atomic<bool> frame_generation{false};
    /// Whether the GPU can (set by the renderer), or why not.
    std::atomic<bool> frame_generation_supported{false};
    std::atomic<const char*> frame_generation_problem{nullptr};
    /// Why FSR 4 cannot run (assets, device features), or null. Set by the renderer.
    std::atomic<const char*> fsr4_problem{nullptr};
    std::atomic<bool> fsr4_supported{false}, fsr411_supported{false};
    /// DLSS (gpu/dlss_bridge, NVIDIA RTX) is ready, or why not (null before the device exists).
    std::atomic<bool> dlss_supported{false};
    std::atomic<const char*> dlss_problem{nullptr};
    /// Mouse camera controls (live-reloaded in src/runtime_pad.c).
    std::atomic<bool> mouse_look{true};
    std::atomic<bool> mouse_invert_y{false};
    std::atomic<float> mouse_sensitivity{1.0f};
    /// DLSS model preset: 11 = DLSS 4 Transformer (Preset K), 0 = DLSS 3 CNN Default.
    std::atomic<int> dlss_preset{11};
    /// Forced anisotropic filtering on scene textures: 16, 8, 4, or 0 (game default).
    std::atomic<int> anisotropic{16};
    /// Persistent FPS limit (0 = Uncapped, or 30..1000).
    std::atomic<int> fps_limit{0};

    /// Startup settings for the explicit BB_RENDER_RES compatibility patch only.
    int startup_preset = NativeAA;
    int startup_upscaler = UpscalerFsr3;
    bool startup_object_motion = true;
    bool startup_effects[EffectCount]{};
    int startup_model_lod = 0;
    /// The output the startup patch was made for (BB_OUTPUT_RES), else 0x0.
    OutputSize startup_output{0, 0};
    int startup_live_resolution = 0;
};

Values& Get();
/// Localized overlay text; the Language selector itself stays in English.
const char* MenuText(const char* english, const char* russian);

/// Reads the file, then the environment overrides. Called once at start.
void Load();
/// Checks the loaded choice before the first frame; unsupported FSR 4 uses FSR 3.1.
void ConfigureUpscalerSupport(bool fsr4, bool fsr411);
/// After device creation: DLSS availability; a DLSS setting falls back to FSR 3.1 without it.
void ConfigureDlssSupport(bool available, const char* problem);
/// Startup-patched scene dimensions cannot change until run.sh prepares a new image.
bool FixedRenderSession();
int EffectiveUpscaler();
int RenderPreset();
bool ResolutionNeedsRestart();
/// Writes the file (menu changes).
void Save();

/// Render resolution divisor of a preset (1.0 native, 1.5 quality, ...).
float PresetScale(int preset);
const char* PresetName(int preset);
const char* UpscalerName(int upscaler);
const char* AaModeName(int aa);

} // namespace BbSettings
