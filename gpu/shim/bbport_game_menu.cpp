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

#include "bbport_frame_rate.h"
#include "bbport_guest_hooks.h"
#include "bbport_settings.h"
#include "games/bloodborne_menu_pointer.h"
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
/// (page, input, frame_time): processes Up/Down row navigation on an options page.
constexpr u64 OptionsPageNavigate = 0x1b1b520;
/// (page, game_settings): Bloodborne's native System -> Controls page content builder.
constexpr u64 ControlsContent = 0x1b211a0;
/// Menu input button check helpers (const u8* input -> bool).
constexpr u64 IsConfirmPressed = 0x1adfec0;
constexpr u64 IsLeftPressed = 0x1ae0890;
constexpr u64 IsRightPressed = 0x1ae0970;
/// (row): the row shows its value's text (looked up again).
constexpr u64 RowShowValue = 0x1b2b7c0;
/// (out_mc 0x60 bytes, parent_mc 0x60 bytes, fmt, ...): looks up a Scaleform child MovieClip.
constexpr u64 GetSubMovieClip = 0x1adb220;
/// (mc + 0x8, int value): formats an integer into a Scaleform TextField MovieClip.
constexpr u64 SetMovieClipIntText = 0x1ad9e90;
/// Base MovieClipWrapper vtable (+0x10) used when destroying a temporary 0x60-byte MovieClipWrapper.
constexpr u64 MovieClipBaseVtable = 0x539a070;
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
int ActiveOrbisLanguage() {
    static const int lang = [] {
        const char* env = std::getenv("BB_LANGUAGE");
        return (env && *env) ? std::atoi(env) : 1; // 1 = English by default
    }();
    return lang;
}

const char* TranslateForCurrentGameLang(const char* en) {
    const int lang = ActiveOrbisLanguage();
    if (lang == 1 || lang == 18 || lang == 8 || !en) {
        return en;
    }
    struct Entry {
        const char* en;
        const char* fr; // 2
        const char* es; // 3, 20
        const char* de; // 4
        const char* it; // 5
        const char* pt; // 7, 17
        const char* pl; // 16
        const char* ja; // 0
        const char* ko; // 9
        const char* zh; // 10, 11
    };
    static constexpr Entry Table[] = {
        {"Display", "Affichage", "Pantalla", "Anzeige", "Schermo", "Tela", "Ekran", "ディスプレイ", "디스플레이", "显示"},
        {"Effects", "Effets", "Efectos", "Effekte", "Effetti", "Efeitos", "Efekty", "エフェクト", "효과", "特效"},
        {"Aspect Ratio", "Format d'image", "Relación de aspecto", "Seitenverhältnis", "Rapporto d'aspetto", "Proporção de tela", "Proporcje ekranu", "アスペクト比", "화면 비율", "宽高比"},
        {"Resolution", "Résolution", "Resolución", "Auflösung", "Risoluzione", "Resolução", "Rozdzielczość", "解像度", "해상도", "分辨率"},
        {"FPS Limit", "Limite de FPS", "Límite de FPS", "FPS-Limit", "Limite FPS", "Limite de FPS", "Limit FPS", "最大フレームレート", "FPS 제한", "帧率上限"},
        {"Anti-Aliasing", "Anticrénelage", "Antialiasing", "Kantenglättung", "Anti-Aliasing", "Anti-Aliasing", "Wygładzanie krawędzi", "アンチエイリアス", "안티앨리어싱", "抗锯齿"},
        {"Upscaler", "Mise à l'échelle", "Escalador", "Upscaler", "Upscaler", "Upscaler", "Skalowanie", "アップスケーラー", "업스케일러", "超分辨率"},
        {"Upscaler Mode", "Mode d'échelle", "Modo de escalado", "Upscaler-Modus", "Modalità Upscaler", "Modo do Upscaler", "Tryb skalowania", "アップスケールモード", "업스케일 모드", "缩放模式"},
        {"Sharpness (RCAS)", "Netteté (RCAS)", "Nitidez (RCAS)", "Schärfe (RCAS)", "Nitidezza (RCAS)", "Nitidez (RCAS)", "Ostrość (RCAS)", "シャープネス (RCAS)", "선명도 (RCAS)", "锐化 (RCAS)"},
        {"Anisotropic Filtering", "Filtrage anisotrope", "Filtro anisotrópico", "Anisotrope Filterung", "Filtro anisotropico", "Filtro anisotrópico", "Filtrowanie anizotropowe", "異方性フィルタリング", "이방성 필터링", "各向异性过滤"},
        {"Model Detail (LOD)", "Détail des modèles (LOD)", "Detalle de modelos (LOD)", "Modelldetails (LOD)", "Dettaglio modelli (LOD)", "Detalhe dos modelos (LOD)", "Szczegółowość modeli (LOD)", "モデル詳細度 (LOD)", "모델 디테일 (LOD)", "模型细节 (LOD)"},
        {"Uncapped", "Illimité", "Sin límite", "Unbegrenzt", "Illimitato", "Ilimitado", "Bez limitu", "無制限", "제한 없음", "无限制"},
        {"Off", "Désactivé", "Desactivado", "Aus", "Disattivato", "Desativado", "Wył.", "オフ", "꺼짐", "关闭"},
        {"On", "Activé", "Activado", "Ein", "Attivato", "Ativado", "Wł.", "オン", "켜짐", "开启"},
        {"Normal", "Normal", "Normal", "Normal", "Normale", "Normal", "Normalna", "標準", "보통", "正常"},
        {"Inverted", "Inversé", "Invertido", "Invertiert", "Invertito", "Invertido", "Odwrócona", "反転", "반전", "反转"},
        {"Mouse Speed", "Vitesse souris", "Velocidad del ratón", "Mausgeschwindigkeit", "Velocità mouse", "Velocidade do mouse", "Czułość myszy", "マウス感度", "마우스 감도", "鼠标速度"},
        {"Controller Vibration", "Vibrations manette", "Vibración del mando", "Controller-Vibration", "Vibrazione controller", "Vibração do controle", "Wibracje kontrolera", "振動機能", "패드 진동", "手柄震动"},
        {"Camera Sensitivity", "Sensibilité caméra", "Sensibilidad de cámara", "Kameraempfindlichkeit", "Sensibilità telecamera", "Sensibilidade da câmera", "Czułość kamery", "カメラ速度", "카메라 감도", "视角灵敏度"},
        {"Camera X-Axis", "Caméra axe X", "Eje X de cámara", "Kamera X-Achse", "Asse X telecamera", "Eixo X da câmera", "Oś X kamery", "カメラ左右操作", "카메라 좌우 조작", "视角 X 轴"},
        {"Camera Y-Axis", "Caméra axe Y", "Eje Y de cámara", "Kamera Y-Achse", "Asse Y telecamera", "Eixo Y da câmera", "Oś Y kamery", "カメラ上下操作", "카메라 상하 조작", "视角 Y 轴"},
        {"Auto Target Switch", "Changement de cible auto", "Cambio auto. de objetivo", "Auto-Zielwechsel", "Cambio bersaglio auto.", "Troca auto. de alvo", "Auto-zmiana celu", "ターゲット自動変更", "타겟 자동 변경", "自动切换锁定"},
        {"Camera Auto-Adjust", "Ajustement caméra auto", "Ajuste auto. de cámara", "Kamera-Auto-Korrektur", "Regolazione auto. telecamera", "Ajuste auto. da câmera", "Auto-korekta kamery", "カメラ自動補正", "카메라 자동 보정", "视角自动回避"},
        {"Move Forward", "Avancer", "Avanzar", "Vorwärts bewegen", "Muovi avanti", "Mover para frente", "Ruch w przód", "前進", "앞으로 이동", "向前移动"},
        {"Move Back", "Reculer", "Retroceder", "Rückwärts bewegen", "Muovi indietro", "Mover para trás", "Ruch w tył", "後退", "뒤로 이동", "向后移动"},
        {"Move Left", "Aller à gauche", "Mover a la izquierda", "Nach links bewegen", "Muovi a sinistra", "Mover para a esquerda", "Ruch w lewo", "左移動", "왼쪽으로 이동", "向左移动"},
        {"Move Right", "Aller à droite", "Mover a la derecha", "Nach rechts bewegen", "Muovi a destra", "Mover para a direita", "Ruch w prawo", "右移動", "오른쪽으로 이동", "向右移动"},
        {"Dodge / Sprint / Backstep", "Esquive / Sprint / Pas arrière", "Esquivar / Correr / Paso atrás", "Ausweichen / Sprinten / Rückschritt", "Schivata / Scatto / Passo indietro", "Esquiva / Corrida / Passo atrás", "Unik / Sprint / Odskok", "回避 / ダッシュ / バックステップ", "회피 / 질주 / 백스텝", "闪避 / 冲刺 / 后撤步"},
        {"Interact / Confirm", "Interagir / Confirmer", "Interactuar / Confirmar", "Interagieren / Bestätigen", "Interagisci / Conferma", "Interagir / Confirmar", "Interakcja / Potwierdź", "調べる / 決定", "상호작용 / 확인", "互动 / 确认"},
        {"Use Quick Item", "Utiliser objet rapide", "Usar objeto rápido", "Schnellgegenstand benutzen", "Usa oggetto rapido", "Usar item rápido", "Użyj przedmiotu", "アイテム使用", "퀵 아이템 사용", "使用快捷道具"},
        {"Blood Vial (Heal)", "Fiole de sang (Soin)", "Vial de sangre (Curar)", "Blutphiole (Heilen)", "Fiala di sangue (Cura)", "Frasco de sangue (Curar)", "Fiolka krwi (Leczenie)", "輸血液を使用", "수혈액 사용 (회복)", "使用采血瓶（治疗）"},
        {"Light Attack", "Attaque légère", "Ataque ligero", "Leichter Angriff", "Attacco leggero", "Ataque leve", "Szybki atak", "通常攻撃 (R1)", "약공격 (R1)", "轻攻击 (R1)"},
        {"Heavy Attack", "Attaque puissante", "Ataque fuerte", "Schwerer Angriff", "Attacco pesante", "Ataque forte", "Silny atak", "強攻撃 (R2)", "강공격 (R2)", "重攻击 (R2)"},
        {"Firearm Shot / Parry", "Tir d'arme à feu / Parade", "Disparo / Parry", "Schusswaffe / Parieren", "Colpo arma da fuoco / Parry", "Disparo / Parry", "Strzał z broni palnej", "銃撃 / パリィ (L2)", "총격 / 패리 (L2)", "枪击 / 枪反 (L2)"},
        {"Transform Weapon", "Transformer l'arme", "Transformar arma", "Waffe transformieren", "Trasforma arma", "Transformar arma", "Przekształć broń", "武器変形 (L1)", "무기 변형 (L1)", "武器变形 (L1)"},
        {"Lock-On / Reset Camera", "Verrouiller / Réinit. caméra", "Fijar objetivo / Cámara", "Anvisieren / Kamera", "Aggancia / Resetta visuale", "Travar mira / Câmera", "Namierzanie / Kamera", "ターゲット固定 / カメラリセット", "타겟 고정 / 카메라 리셋", "锁定目标 / 重置视角"},
    };
    for (const auto& e : Table) {
        if (std::strcmp(en, e.en) == 0) {
            switch (lang) {
                case 2: return e.fr;
                case 3:
                case 20: return e.es;
                case 4: return e.de;
                case 5: return e.it;
                case 7:
                case 17: return e.pt;
                case 16: return e.pl;
                case 0: return e.ja;
                case 9: return e.ko;
                case 10:
                case 11: return e.zh;
                default: return en;
            }
        }
    }
    return en;
}

u32 AddText(const char* english, const char* russian) {
    texts.push_back({Utf16(TranslateForCurrentGameLang(english)), Utf16(russian)});
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
// the 6 dynamic rows inside the unified Controls page, and the 6 dynamic rows inside Display):
// fixed buffers rewritten in place, so a text the game holds stays valid.
constexpr u32 DynamicBase = IdBase + 0x8000;
constexpr std::size_t DynamicLength = 128;
struct DynamicText {
    char16_t english[DynamicLength]{}, russian[DynamicLength]{};
};
constexpr u32 OutputSlots = 12;   ///< BbSettings::OutputList
constexpr int ControlsSlotCount = 6; ///< all 6 visible rows in Controls scroll across 20 items
constexpr int TotalControlsItems = BbSettings::KeyActionCount + 7; ///< 13 keybinds + Mouse Speed + 6 native settings
constexpr int MaxControlsScroll = TotalControlsItems - ControlsSlotCount;
constexpr int DisplaySlotCount = 6; ///< all 6 visible rows in Display scroll across 9 items
constexpr int TotalDisplayItems = 9;
constexpr int MaxDisplayScroll = TotalDisplayItems - DisplaySlotCount;
constexpr int MaxChoicesPerSlot = 16;
constexpr int DefaultResetSentinel = -999;
std::array<DynamicText, 1 + OutputSlots + 3 * ControlsSlotCount + 3 * DisplaySlotCount +
                        ControlsSlotCount * 4 + DisplaySlotCount * MaxChoicesPerSlot>
    dynamic_texts{};
constexpr u32 AutoText = DynamicBase;                                       ///< "Auto (21:9)"
constexpr u32 OutputTexts = DynamicBase + 1;                                ///< the resolutions (OutputSlots)
constexpr u32 ControlsSlotName = OutputTexts + OutputSlots;                 ///< 6 dynamic row labels in Controls
constexpr u32 ControlsSlotHelp = ControlsSlotName + ControlsSlotCount;      ///< 6 dynamic row helps in Controls
constexpr u32 ControlsSlotVal = ControlsSlotHelp + ControlsSlotCount;       ///< 6 dynamic row values in Controls
constexpr u32 DisplaySlotName = ControlsSlotVal + ControlsSlotCount;        ///< 6 dynamic row labels in Display
constexpr u32 DisplaySlotHelp = DisplaySlotName + DisplaySlotCount;         ///< 6 dynamic row helps in Display
constexpr u32 DisplaySlotVal = DisplaySlotHelp + DisplaySlotCount;          ///< 6 dynamic row values in Display
constexpr u32 ControlsSlotChoices = DisplaySlotVal + DisplaySlotCount;      ///< 6 * 4 choice texts in Controls
constexpr u32 DisplaySlotChoices = ControlsSlotChoices + ControlsSlotCount * 4; ///< 6 * 16 choice texts in Display

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
    Store16(t.english, TranslateForCurrentGameLang(english));
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
    FirstSlot = FirstEffect + BbSettings::EffectCount,
    FirstDisplaySlot = FirstSlot + ControlsSlotCount,
    FieldCount = int(FirstDisplaySlot) + DisplaySlotCount
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

void SyncDisplayStateFromSettings();

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
    for (int slot = 0; slot < ControlsSlotCount; ++slot) {
        values[FirstSlot + slot] = 0;
    }
    for (int slot = 0; slot < DisplaySlotCount; ++slot) {
        values[FirstDisplaySlot + slot] = 0;
    }
}

void LoadValues() {
    const int aspect = BbSettings::Get().aspect;
    menu_outputs = BbSettings::Outputs(aspect);
    RefreshOutputs(aspect);
    Fill(BbSettings::Get(), values);
    SyncDisplayStateFromSettings();
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

    // Scrollable 9-item Display page (Aspect Ratio, Resolution, FPS Limit, Anti-Aliasing,
    // Upscaler, Upscaler Mode, Sharpness, Anisotropic Filtering, Model Detail):
    page("Display", "Изображение",
         "Resolution, FPS limit, Anti-Aliasing (DLAA/TAA/FXAA), Upscaler (DLSS 4/FSR) & Filtering",
         "Разрешение, лимит FPS, сглаживание (DLAA/TAA/FXAA), апскейлер (DLSS 4/FSR) и фильтрация",
         ControlsLayout);
    for (int slot = 0; slot < DisplaySlotCount; ++slot) {
        pages.back().rows.push_back(
            {Field(FirstDisplaySlot + slot),
             DisplaySlotName + u32(slot),
             DisplaySlotHelp + u32(slot),
             {DisplaySlotVal + u32(slot), DisplaySlotVal + u32(slot)},
             Kind::Select});
    }

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

    // Unified System -> Controls window: all 6 visible rows scroll vertically across all 20 items
    // (13 Keybinds + Mouse Speed + Controller Vibration + Camera Sensitivity + 4 Camera toggles).
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
void InitDisplayPage(void* page);

// Called by the game's menu code (guest threads, System V ABI like the game), one set per page.
template <int P>
void PS4_SYSV_ABI Rows(void* page, void*) {
    if constexpr (P == 0) {
        InitDisplayPage(page);
    } else {
        LoadValues();
        AddRows(page, pages[P].rows);
    }
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
bool capturing_alt = false; ///< false: Primary Key 1, true: Alt Key 2
CapturePhase capture_phase = CapturePhase::WaitInitialRelease;
std::vector<std::string> captured_tokens;
std::string last_capture_preview;
std::atomic<long long> capture_cooldown_ms{0};
std::atomic<long long> last_capture_step_ms{0};

u8* active_controls_page = nullptr;
u8* active_game_settings = nullptr;
u8* slot_rows[ControlsSlotCount] = {};
int controls_scroll = 0;
bool controls_wrapped_from_top = false;

u8* active_display_page = nullptr;
u8* display_slot_rows[DisplaySlotCount] = {};
int display_scroll = 0;
bool display_wrapped_from_top = false;

int GetSelectedPageRow(u8* page) {
    if (!page) return 0;
    int sel = *reinterpret_cast<int*>(page + 0xae8);
    if (sel < 0) sel = *reinterpret_cast<int*>(page + 0xae4);
    return sel;
}

int GetSelectedControlsRow() {
    return GetSelectedPageRow(active_controls_page);
}

int GetSelectedDisplayRow() {
    return GetSelectedPageRow(active_display_page);
}

long long NowSteadyMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

bool IsModifierToken(const std::string& tok) {
    return tok == "Left Shift" || tok == "Right Shift" || tok == "Left Ctrl" ||
           tok == "Right Ctrl" || tok == "Left Alt" || tok == "Right Alt";
}

void SplitBinding(const std::string& raw, std::string& primary, std::string& alt) {
    const std::size_t comma = raw.find(',');
    if (comma == std::string::npos) {
        primary = raw;
        alt.clear();
    } else {
        primary = raw.substr(0, comma);
        alt = raw.substr(comma + 1);
    }
}

std::string JoinBinding(const std::string& primary, const std::string& alt) {
    if (primary.empty()) return alt;
    if (alt.empty() || alt == primary) return primary;
    return primary + "," + alt;
}

/// True when the menu Confirm signal was caused by pressing the bare keyboard `F` key
/// (as opposed to Enter, Mouse Left Click, or Gamepad Cross).
bool IsBareKeyboardF() {
    if (!SDL_WasInit(SDL_INIT_VIDEO)) return false;
    int num_keys = 0;
    const bool* keys = SDL_GetKeyboardState(&num_keys);
    if (!keys || num_keys <= SDL_SCANCODE_F || !keys[SDL_SCANCODE_F]) return false;
    if (num_keys > SDL_SCANCODE_RETURN && keys[SDL_SCANCODE_RETURN]) return false;
    if (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK) return false;
    return true;
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

/// Returns keybind action index (0..12) for item_idx (0..19), or -1 if it's a slider/toggle setting.
int ItemKeyAction(int item_idx) {
    return (item_idx >= 0 && item_idx < BbSettings::KeyActionCount) ? item_idx : -1;
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

void RefreshSlotTexts(int slot) {
    const int item_idx = std::clamp(controls_scroll + slot, 0, TotalControlsItems - 1);
    const u32 name_id = ControlsSlotName + u32(slot);
    const u32 help_id = ControlsSlotHelp + u32(slot);
    const u32 val_id = ControlsSlotVal + u32(slot);

    const int k = ItemKeyAction(item_idx);
    if (k >= 0) {
        const auto& def = BbSettings::GetKeyActionDef(k);
        SetDynamic(name_id, def.name_en, def.name_ru);
        const std::string help_en = std::string(def.help_en) + "  [Click / Enter: Rebind | Right Arrow: +Alt Key]";
        const std::string help_ru = std::string(def.help_ru) + "  [Клик / Enter: Назначить | Вправо: +Альт. кл.]";
        SetDynamic(help_id, help_en.c_str(), help_ru.c_str());
        const std::string raw = BbSettings::GetKeyBinding(k);
        const std::string en = BbSettings::FormatKeyBinding(raw, false);
        const std::string ru = BbSettings::FormatKeyBinding(raw, true);
        SetDynamic(val_id, en.c_str(), ru.c_str());
        return;
    }

    if (item_idx == 13) {
        SetDynamic(name_id, "Mouse Speed", "Чувствит. мыши");
        SetDynamic(help_id,
                   "Mouse camera sensitivity (Left/Right): 0 mouse look off, 5 default (1.0x), 10 fast (2.0x)",
                   "Чувствительность камеры мыши (Влево/Вправо): 0 выкл, 5 обычная (1.0x), 10 быстрая (2.0x)");
        const int step = CurrentMouseSpeedStep();
        const std::string txt = fmt::format("{}", step);
        SetDynamic(val_id, txt.c_str(), txt.c_str());
        return;
    }

    if (item_idx == 14) {
        SetDynamic(name_id, "Controller Vibration", "Вибрация геймпада");
        SetDynamic(help_id,
                   "Controller vibration strength (Left/Right): 0 off, 10 strongest",
                   "Сила вибрации геймпада (Влево/Вправо): 0 выкл, 10 макс.");
        const int vib = active_game_settings ? std::clamp(int(active_game_settings[0x1]), 0, 10) : 10;
        const std::string txt = fmt::format("{}", vib);
        SetDynamic(val_id, txt.c_str(), txt.c_str());
        return;
    }

    if (item_idx == 15) {
        SetDynamic(name_id, "Camera Sensitivity", "Скорость камеры");
        SetDynamic(help_id, "Gamepad & lock-on camera rotation speed (Left/Right: 0 .. 10)",
                   "Скорость вращения камеры (Влево/Вправо: 0 .. 10)");
        const int sens = active_game_settings ? std::clamp(int(active_game_settings[0x0]), 0, 10) : 5;
        const std::string txt = fmt::format("{}", sens);
        SetDynamic(val_id, txt.c_str(), txt.c_str());
        return;
    }

    if (item_idx == 16) {
        SetDynamic(name_id, "Camera X-Axis", "Камера по оси X");
        SetDynamic(help_id, "Horizontal camera rotation direction (Left/Right to toggle)",
                   "Направление вращения камеры по горизонтали (Влево/Вправо)");
        const bool inv = active_game_settings && active_game_settings[0xa] == 0;
        SetDynamic(val_id, inv ? "Inverted" : "Normal", inv ? "Инверсия" : "Обычная");
    } else if (item_idx == 17) {
        SetDynamic(name_id, "Camera Y-Axis", "Камера по оси Y");
        SetDynamic(help_id, "Vertical camera rotation direction (Left/Right to toggle)",
                   "Направление вращения камеры по вертикали (Влево/Вправо)");
        const bool inv = active_game_settings && active_game_settings[0x9] == 0;
        SetDynamic(val_id, inv ? "Inverted" : "Normal", inv ? "Инверсия" : "Обычная");
    } else if (item_idx == 18) {
        SetDynamic(name_id, "Auto Target Switch", "Автосмена цели");
        SetDynamic(help_id, "Automatically switch lock-on target when current target is defeated",
                   "Автоматическое переключение захвата цели после гибели текущей цели");
        const bool on = !active_game_settings || active_game_settings[0xb] != 0;
        SetDynamic(val_id, on ? "On" : "Off", on ? "Вкл" : "Выкл");
    } else if (item_idx == 19) {
        SetDynamic(name_id, "Camera Auto-Adjust", "Автокоррекция камеры");
        SetDynamic(help_id, "Automatically adjust camera when colliding with walls",
                   "Автоматический обход стен при движении камеры");
        const bool on = !active_game_settings || active_game_settings[0xc] != 0;
        SetDynamic(val_id, on ? "On" : "Off", on ? "Вкл" : "Выкл");
    }
}

int GetItemSliderValue(int item_idx) {
    if (item_idx == 13) {
        return CurrentMouseSpeedStep();
    }
    if (item_idx == 14) {
        return active_game_settings ? std::clamp(int(active_game_settings[0x1]), 0, 10) : 10;
    }
    if (item_idx == 15) {
        return active_game_settings ? std::clamp(int(active_game_settings[0x0]), 0, 10) : 5;
    }
    return -1;
}

void SetMovieClipVisible(u8* mc, int visible) {
    const u64 vtbl = *reinterpret_cast<u64*>(mc);
    if (!vtbl) return;
    auto fn = *reinterpret_cast<void (PS4_SYSV_ABI**)(void*, int)>(vtbl + 0x8);
    if (fn) fn(mc, visible);
}

void DestroyMovieClip(u8* mc) {
    *reinterpret_cast<u64*>(mc) = image_base + MovieClipBaseVtable;
    const u32 flags = *reinterpret_cast<u32*>(mc + 0x48);
    if (flags & 0x40) {
        void* mgr = *reinterpret_cast<void**>(mc + 0x40);
        const u64 val = *reinterpret_cast<u64*>(mc + 0x50);
        if (mgr) {
            const u64 vtbl = *reinterpret_cast<u64*>(mgr);
            auto rel = *reinterpret_cast<void (PS4_SYSV_ABI**)(void*, void*, u64)>(vtbl + 0x18);
            rel(mgr, mc + 0x30, val);
        }
        *reinterpret_cast<u64*>(mc + 0x40) = 0;
    }
    *reinterpret_cast<u32*>(mc + 0x48) = 0;
}

void SyncPageSlotSliderWidget(u8* page, u8* row, int slot, int slider_val, bool is_focused) {
    if (!row || !page) return;
    using GetSubFn = void (PS4_SYSV_ABI*)(void*, const void*, const char*, ...);
    auto get_sub = Game<GetSubFn>(GetSubMovieClip);

    alignas(16) u8 item_mc[0x60] = {};
    alignas(16) u8 slider_mc[0x60] = {};
    alignas(16) u8 combo_mc[0x60] = {};
    get_sub(item_mc, page + 0xdf0, "Item_%d_0", slot);
    get_sub(slider_mc, item_mc, "Widgets/Slider");
    get_sub(combo_mc, item_mc, "Widgets/ComboBox");

    if (slider_val >= 0) {
        SetMovieClipVisible(row + 0x10, 0);
        SetMovieClipVisible(combo_mc, 0);
        SetMovieClipVisible(slider_mc, 1);

        alignas(16) u8 cursor_mc[0x60] = {};
        get_sub(cursor_mc, slider_mc, "Cursor");
        SetMovieClipVisible(cursor_mc, is_focused ? 1 : 0);
        DestroyMovieClip(cursor_mc);

        alignas(16) u8 left_mc[0x60] = {};
        alignas(16) u8 right_mc[0x60] = {};
        get_sub(left_mc, slider_mc, "LeftArrow");
        get_sub(right_mc, slider_mc, "RightArrow");
        SetMovieClipVisible(left_mc, (is_focused && slider_val > 0) ? 1 : 0);
        SetMovieClipVisible(right_mc, (is_focused && slider_val < 10) ? 1 : 0);
        DestroyMovieClip(right_mc);
        DestroyMovieClip(left_mc);

        alignas(16) u8 text_mc[0x60] = {};
        get_sub(text_mc, slider_mc, "Text_0");
        Game<void (PS4_SYSV_ABI*)(void*, int)>(SetMovieClipIntText)(text_mc + 0x8, slider_val);
        DestroyMovieClip(text_mc);

        alignas(16) u8 bar_mc[0x60] = {};
        get_sub(bar_mc, slider_mc, "Bar");
        void* bar_inner = *reinterpret_cast<void**>(bar_mc + 0x18);
        if (bar_inner) {
            alignas(16) u8 gfval[0x30] = {};
            auto get_val = **reinterpret_cast<void (PS4_SYSV_ABI***)(void*, void*)>(bar_inner);
            get_val(gfval, bar_inner);
            const u32 flags = *reinterpret_cast<u32*>(gfval + 0x20);
            if (flags & 0x8f) {
                void* mgr = *reinterpret_cast<void**>(gfval + 0x18);
                const u64 obj = *reinterpret_cast<u64*>(gfval + 0x28);
                if (mgr) {
                    const u64 vtbl = *reinterpret_cast<u64*>(mgr);
                    auto goto_and_stop =
                        *reinterpret_cast<void (PS4_SYSV_ABI**)(void*, u64, u64, u32)>(vtbl + 0x158);
                    goto_and_stop(mgr, obj, u64(1 + std::clamp(slider_val, 0, 10) * 10), 1);
                }
            }
            const u32 flags_after = *reinterpret_cast<u32*>(gfval + 0x20);
            if (flags_after & 0x40) {
                void* mgr = *reinterpret_cast<void**>(gfval + 0x18);
                const u64 obj = *reinterpret_cast<u64*>(gfval + 0x28);
                if (mgr) {
                    const u64 vtbl = *reinterpret_cast<u64*>(mgr);
                    auto rel = *reinterpret_cast<void (PS4_SYSV_ABI**)(void*, void*, u64)>(vtbl + 0x18);
                    rel(mgr, gfval + 0x08, obj);
                }
                *reinterpret_cast<u64*>(gfval + 0x18) = 0;
            }
        }
        DestroyMovieClip(bar_mc);
    } else {
        SetMovieClipVisible(slider_mc, 0);
        SetMovieClipVisible(combo_mc, 0);
        SetMovieClipVisible(row + 0x10, 1);
    }

    DestroyMovieClip(combo_mc);
    DestroyMovieClip(slider_mc);
    DestroyMovieClip(item_mc);
}

void SyncSlotSliderWidget(u8* row, int slot, bool is_focused) {
    const int item_idx = std::clamp(controls_scroll + slot, 0, TotalControlsItems - 1);
    SyncPageSlotSliderWidget(active_controls_page, row, slot, GetItemSliderValue(item_idx), is_focused);
}

void PopulateSelectRowChoices(u8* row, int field_idx, const std::vector<u32>& choice_ids, int selected_idx) {
    if (!row || choice_ids.empty()) return;
    u8* items = row + RowValues;
    items += (0 - reinterpret_cast<std::uintptr_t>(items)) & 7;
    u64& count = *reinterpret_cast<u64*>(row + RowCount);
    const u64 want = std::min<u64>(choice_ids.size(), 32);
    const bool in_place = *reinterpret_cast<u64*>(items + 0x30) < 8;
    if (count > 0 && want <= 32 && (want <= count || in_place)) {
        for (u64 i = count; i < want; ++i) {
            std::memcpy(items + i * ValueSize, items, ValueSize);
        }
        for (u64 i = 0; i < want; ++i) {
            *reinterpret_cast<u32*>(items + i * ValueSize) = u32(i);
            *reinterpret_cast<const char16_t**>(items + i * ValueSize + 8) = TextOf(choice_ids[i]);
        }
        count = want;
        Game<void (PS4_SYSV_ABI*)(void*, u32)>(ListSetCount)(row + RowList, u32(want));
    }
    const int clamped = std::clamp(selected_idx, 0, int(want) - 1);
    values[field_idx] = clamped;
    applied[field_idx] = clamped;
    default_values[field_idx] = DefaultResetSentinel;
    *reinterpret_cast<int*>(row + RowDefault) = DefaultResetSentinel;
    Game<int (PS4_SYSV_ABI*)(void*, u32)>(ListSelect)(row + RowList, u32(clamped));
    Game<void (PS4_SYSV_ABI*)(void*)>(ListShow)(row + RowList);
    Game<void (PS4_SYSV_ABI*)(void*)>(RowShowValue)(row);
    Game<void (PS4_SYSV_ABI*)(void*)>(ListDraw)(row + RowList);
}

void BuildControlsSlotChoices(int slot, std::vector<u32>& out_choices, int& out_selected_idx) {
    out_choices.clear();
    out_selected_idx = 0;
    const int item_idx = std::clamp(controls_scroll + slot, 0, TotalControlsItems - 1);
    const u32 base_id = ControlsSlotChoices + u32(slot * 4);
    auto add_choice = [&](const char* en, const char* ru) {
        const u32 id = base_id + u32(out_choices.size());
        SetDynamic(id, en, ru);
        out_choices.push_back(id);
    };
    if (item_idx == 16) {
        add_choice("Normal", "Обычная");
        add_choice("Inverted", "Инверсия");
        out_selected_idx = (active_game_settings && active_game_settings[0xa] == 0) ? 1 : 0;
    } else if (item_idx == 17) {
        add_choice("Normal", "Обычная");
        add_choice("Inverted", "Инверсия");
        out_selected_idx = (active_game_settings && active_game_settings[0x9] == 0) ? 1 : 0;
    } else if (item_idx == 18) {
        add_choice("Off", "Выкл");
        add_choice("On", "Вкл");
        out_selected_idx = (!active_game_settings || active_game_settings[0xb] != 0) ? 1 : 0;
    } else if (item_idx == 19) {
        add_choice("Off", "Выкл");
        add_choice("On", "Вкл");
        out_selected_idx = (!active_game_settings || active_game_settings[0xc] != 0) ? 1 : 0;
    }
}

void UpdateSlotRowDisplay(u8* row, int slot) {
    if (!row) return;
    const int item_idx = std::clamp(controls_scroll + slot, 0, TotalControlsItems - 1);
    if (item_idx >= 16) {
        std::vector<u32> choice_ids;
        int selected_idx = 0;
        BuildControlsSlotChoices(slot, choice_ids, selected_idx);
        PopulateSelectRowChoices(row, FirstSlot + slot, choice_ids, selected_idx);
    } else {
        const std::vector<u32> single = {ControlsSlotVal + u32(slot), ControlsSlotVal + u32(slot)};
        PopulateSelectRowChoices(row, FirstSlot + slot, single, 0);
    }
    const int sel = GetSelectedControlsRow();
    SyncSlotSliderWidget(row, slot, slot == sel);
}

void RefreshControlsPage() {
    controls_scroll = std::clamp(controls_scroll, 0, MaxControlsScroll);
    for (int slot = 0; slot < ControlsSlotCount; ++slot) {
        RefreshSlotTexts(slot);
    }
    if (active_controls_page) {
        u8* const rows_begin = *reinterpret_cast<u8**>(active_controls_page + 0xe58);
        u8* const rows_end = *reinterpret_cast<u8**>(active_controls_page + 0xe60);
        if (rows_begin && rows_end && rows_end - rows_begin >= ControlsSlotCount * 0x90) {
            for (int slot = 0; slot < ControlsSlotCount; ++slot) {
                u8* const entry = rows_begin + slot * 0x90;
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

void ApplyControlsSlotChoiceIndex(u8* row, int slot, int choice_idx) {
    const int item_idx = std::clamp(controls_scroll + slot, 0, TotalControlsItems - 1);
    const int idx = std::clamp(choice_idx, 0, 1);
    if (item_idx == 16 && active_game_settings) {
        active_game_settings[0xa] = idx == 0 ? 1 : 0; // 1 = Normal, 0 = Inverted
    } else if (item_idx == 17 && active_game_settings) {
        active_game_settings[0x9] = idx == 0 ? 1 : 0; // 1 = Normal, 0 = Inverted
    } else if (item_idx == 18 && active_game_settings) {
        active_game_settings[0xb] = idx == 1 ? 1 : 0; // 1 = On, 0 = Off
    } else if (item_idx == 19 && active_game_settings) {
        active_game_settings[0xc] = idx == 1 ? 1 : 0; // 1 = On, 0 = Off
    }
    RefreshSlotTexts(slot);
    UpdateSlotRowDisplay(row, slot);
}

void StepSlotSetting(u8* row, int slot, int delta, bool wrap) {
    const int item_idx = std::clamp(controls_scroll + slot, 0, TotalControlsItems - 1);
    if (item_idx == 13) {
        int step = CurrentMouseSpeedStep() + delta;
        step = wrap ? (step + MouseSensSteps) % MouseSensSteps : std::clamp(step, 0, MouseSensSteps - 1);
        SetMouseSpeedStep(step);
    } else if (item_idx == 14 && active_game_settings) {
        int vib = int(active_game_settings[0x1]) + delta;
        vib = wrap ? (vib + 11) % 11 : std::clamp(vib, 0, 10);
        active_game_settings[0x1] = u8(vib);
    } else if (item_idx == 15 && active_game_settings) {
        int sens = int(active_game_settings[0x0]) + delta;
        sens = wrap ? (sens + 11) % 11 : std::clamp(sens, 0, 10);
        active_game_settings[0x0] = u8(sens);
    } else if (item_idx == 16 && active_game_settings) {
        active_game_settings[0xa] = active_game_settings[0xa] ? 0 : 1;
    } else if (item_idx == 17 && active_game_settings) {
        active_game_settings[0x9] = active_game_settings[0x9] ? 0 : 1;
    } else if (item_idx == 18 && active_game_settings) {
        active_game_settings[0xb] = active_game_settings[0xb] ? 0 : 1;
    } else if (item_idx == 19 && active_game_settings) {
        active_game_settings[0xc] = active_game_settings[0xc] ? 0 : 1;
    }
    PlayMenuClickSound();
    RefreshSlotTexts(slot);
    UpdateSlotRowDisplay(row, slot);
}

// ---- Scrollable 9-item System -> Display Window ----
constexpr int GameAaEffectIdx = 4; // "effect_game_aa" in BbSettings::Effects
constexpr int FpsPresets[] = {0, 30, 60, 72, 90, 120, 144, 165, 170, 240, 360};
constexpr int FpsPresetCount = int(sizeof(FpsPresets) / sizeof(FpsPresets[0]));
constexpr int AnisoValues[] = {1, 4, 8, 16};
constexpr int AnisoValueCount = 4;

enum NativeAaMode : int {
    AaOff = 0,
    AaFxaa = 1,
    AaTaa = 2,
    AaDlaa = 3,
};

enum DisplayUpscalerChoice : int {
    DispUpOff = 0,
    DispUpDlss4 = 1,   // Preset K = 11 (DLSS 4.0 Transformer)
    DispUpDlss45M = 2, // Preset M = 13 (DLSS 4.5 2nd-Gen Transformer)
    DispUpDlss45L = 3, // Preset L = 12 (DLSS 4.5 2nd-Gen Transformer Ultra)
    DispUpDlss3 = 4,   // Preset 0 (Driver Default / Auto)
    DispUpFsr31 = 5,
    DispUpFsr4 = 6,
    DispUpFsr411 = 7,
};

int disp_aspect = BbSettings::AspectAuto;
int disp_output_idx = 0;
int disp_aa_mode = AaTaa;
DisplayUpscalerChoice disp_upscaler_choice = DispUpOff;
int disp_upscale_preset = BbSettings::Quality;
std::atomic<bool> display_heavy_dirty{false};
std::atomic<long long> display_heavy_dirty_ms{0};

void MarkDisplayHeavyDirty() {
    display_heavy_dirty_ms.store(NowSteadyMs(), std::memory_order_relaxed);
    display_heavy_dirty.store(true, std::memory_order_release);
}

void SyncDisplayStateFromSettings() {
    if (display_heavy_dirty.load(std::memory_order_acquire)) {
        return;
    }
    const auto& s = BbSettings::Get();
    disp_aspect = std::clamp(s.aspect.load(), 0, BbSettings::AspectCount - 1);
    menu_outputs = BbSettings::Outputs(disp_aspect);
    disp_output_idx = MenuOutputIndex(
        disp_aspect, BbSettings::NearestOutput(disp_aspect, BbSettings::UnpackSize(s.output_res.load())));
    const int up = std::clamp(s.upscaler.load(), 0, BbSettings::UpscalerCount - 1);
    const int dlss_p = s.dlss_preset.load();
    const int preset = std::clamp(s.preset.load(), 0, BbSettings::PresetCount - 1);
    const bool fxaa = s.effects[GameAaEffectIdx].load();

    disp_upscale_preset = preset;
    if (up == BbSettings::UpscalerOff || up == BbSettings::UpscalerTaa) {
        disp_upscaler_choice = DispUpOff;
        if (up == BbSettings::UpscalerTaa) {
            disp_aa_mode = AaTaa;
        } else {
            disp_aa_mode = fxaa ? AaFxaa : AaOff;
        }
    } else {
        if (up == BbSettings::UpscalerDlss) {
            disp_upscaler_choice = dlss_p == 13   ? DispUpDlss45M
                                   : dlss_p == 12 ? DispUpDlss45L
                                   : dlss_p == 0  ? DispUpDlss3
                                                  : DispUpDlss4;
            if (preset == BbSettings::NativeAA) {
                disp_aa_mode = AaDlaa;
            }
        } else if (up == BbSettings::UpscalerFsr4) {
            disp_upscaler_choice = DispUpFsr4;
        } else if (up == BbSettings::UpscalerFsr411) {
            disp_upscaler_choice = DispUpFsr411;
        } else {
            disp_upscaler_choice = DispUpFsr31;
        }
    }
}

int CurrentSharpnessStep() {
    const auto& s = BbSettings::Get();
    return s.sharpen
               ? std::clamp(int(std::lround(s.sharpness / SharpnessStep)), 1, SharpnessSteps - 1)
               : 0;
}

void SetSharpnessStep(int step) {
    auto& s = BbSettings::Get();
    step = std::clamp(step, 0, SharpnessSteps - 1);
    s.sharpen = step != 0;
    if (step != 0) {
        s.sharpness = step * SharpnessStep;
    }
    values[Sharpness] = step;
    applied[Sharpness] = step;
    bytes[Sharpness] = u8(step);
    BbSettings::Save();
}

int GetDisplayItemSliderValue(int item_idx) {
    if (item_idx == 6) {
        return CurrentSharpnessStep();
    }
    return -1;
}

void SyncDisplaySlotSliderWidget(u8* row, int slot, bool is_focused) {
    const int item_idx = std::clamp(display_scroll + slot, 0, TotalDisplayItems - 1);
    SyncPageSlotSliderWidget(active_display_page, row, slot, GetDisplayItemSliderValue(item_idx), is_focused);
}

std::vector<int> BuildFpsValuesList(int* out_cur_idx) {
    std::vector<int> vals(std::begin(FpsPresets), std::end(FpsPresets));
    const int cur = std::max(0, BbFrameRate::Limit());
    if (std::find(vals.begin(), vals.end(), cur) == vals.end() && int(vals.size()) < MaxChoicesPerSlot) {
        vals.push_back(cur);
        std::sort(vals.begin(), vals.end());
    }
    int cur_idx = 0;
    for (int i = 0; i < int(vals.size()); ++i) {
        if (vals[i] == cur) {
            cur_idx = i;
            break;
        }
    }
    if (out_cur_idx) *out_cur_idx = cur_idx;
    return vals;
}

std::vector<DisplayUpscalerChoice> GetAvailableUpscalerChoices() {
    const auto& s = BbSettings::Get();
    std::vector<DisplayUpscalerChoice> choices = {DispUpOff};
    if (s.dlss_supported.load()) {
        choices.push_back(DispUpDlss45M);
        choices.push_back(DispUpDlss45L);
        choices.push_back(DispUpDlss4);
        choices.push_back(DispUpDlss3);
    }
    choices.push_back(DispUpFsr31);
    if (!s.dlss_supported.load() || disp_upscaler_choice == DispUpFsr4 || disp_upscaler_choice == DispUpFsr411) {
        if (s.fsr4_supported.load()) choices.push_back(DispUpFsr4);
        if (s.fsr411_supported.load()) choices.push_back(DispUpFsr411);
    }
    return choices;
}

void UpscalerChoiceLabels(DisplayUpscalerChoice c, const char*& out_en, const char*& out_ru) {
    switch (c) {
    case DispUpOff:
        out_en = "Off (Native 100%)";
        out_ru = "Выкл (Натив 100%)";
        break;
    case DispUpDlss45M:
        out_en = "DLSS 4.5 (Preset M)";
        out_ru = "DLSS 4.5 (Preset M)";
        break;
    case DispUpDlss45L:
        out_en = "DLSS 4.5 (Preset L)";
        out_ru = "DLSS 4.5 (Preset L)";
        break;
    case DispUpDlss4:
        out_en = "DLSS 4 (Preset K)";
        out_ru = "DLSS 4 (Preset K)";
        break;
    case DispUpDlss3:
        out_en = "DLSS (Auto Preset)";
        out_ru = "DLSS (Авто пресет)";
        break;
    case DispUpFsr4:
        out_en = "FSR 4";
        out_ru = "FSR 4";
        break;
    case DispUpFsr411:
        out_en = "FSR 4.1.1";
        out_ru = "FSR 4.1.1";
        break;
    case DispUpFsr31:
    default:
        out_en = "FSR 3.1";
        out_ru = "FSR 3.1";
        break;
    }
}

void UpscalerModeLabels(int preset, const char*& out_en, const char*& out_ru) {
    switch (std::clamp(preset, int(BbSettings::NativeAA), int(BbSettings::UltraPerformance))) {
    case BbSettings::NativeAA:
        out_en = "Native 100% (DLAA)";
        out_ru = "Натив 100% (DLAA)";
        break;
    case BbSettings::Quality:
        out_en = "Quality (67% Scale)";
        out_ru = "Качество (67%)";
        break;
    case BbSettings::Balanced:
        out_en = "Balanced (58% Scale)";
        out_ru = "Баланс (58%)";
        break;
    case BbSettings::Performance:
        out_en = "Performance (50% Scale)";
        out_ru = "Производит. (50%)";
        break;
    case BbSettings::UltraPerformance:
    default:
        out_en = "Ultra Perf. (33% Scale)";
        out_ru = "Ультра-произв. (33%)";
        break;
    }
}

void BuildDisplaySlotChoices(int slot, std::vector<u32>& out_choices, int& out_selected_idx) {
    out_choices.clear();
    out_selected_idx = 0;
    const int item_idx = std::clamp(display_scroll + slot, 0, TotalDisplayItems - 1);
    const u32 base_id = DisplaySlotChoices + u32(slot * MaxChoicesPerSlot);
    const auto& s = BbSettings::Get();
    auto add_choice = [&](const char* en, const char* ru) {
        if (int(out_choices.size()) >= MaxChoicesPerSlot) return;
        const u32 id = base_id + u32(out_choices.size());
        SetDynamic(id, en, ru);
        out_choices.push_back(id);
    };

    if (item_idx == 0) {
        const std::string group = BbSettings::AspectKey(BbSettings::ResolvedAspect(BbSettings::AspectAuto));
        add_choice(("Auto (" + group + ")").c_str(), ("Авто (" + group + ")").c_str());
        for (int a = BbSettings::AspectAuto + 1; a < BbSettings::AspectCount; ++a) {
            const char* k = BbSettings::AspectKey(a);
            add_choice(k, k);
        }
        out_selected_idx = std::clamp(disp_aspect, 0, int(out_choices.size()) - 1);
        return;
    }

    if (item_idx == 1) {
        const int n = std::clamp(menu_outputs.count, 1, MaxChoicesPerSlot);
        for (int i = 0; i < n; ++i) {
            const auto o = MenuOutput(disp_aspect, i);
            const std::string txt = fmt::format("{} x {}", o.width, o.height);
            add_choice(txt.c_str(), txt.c_str());
        }
        out_selected_idx = std::clamp(disp_output_idx, 0, int(out_choices.size()) - 1);
        return;
    }

    if (item_idx == 2) {
        int cur_idx = 0;
        const std::vector<int> fps_vals = BuildFpsValuesList(&cur_idx);
        for (int fps : fps_vals) {
            if (fps <= 0) {
                add_choice("Uncapped", "Без лимита");
            } else {
                bool is_preset = false;
                for (int p : FpsPresets) {
                    if (p == fps) is_preset = true;
                }
                const std::string en = is_preset ? fmt::format("{} FPS", fps) : fmt::format("{} FPS (Custom)", fps);
                const std::string ru = is_preset ? fmt::format("{} FPS", fps) : fmt::format("{} FPS (Свой)", fps);
                add_choice(en.c_str(), ru.c_str());
            }
        }
        out_selected_idx = std::clamp(cur_idx, 0, int(out_choices.size()) - 1);
        return;
    }

    if (item_idx == 3) {
        add_choice("Off", "Выкл");
        add_choice("FXAA", "FXAA");
        add_choice("TAA", "TAA");
        if (s.dlss_supported.load()) {
            add_choice("DLAA (DLSS 4)", "DLAA (DLSS 4)");
        }
        out_selected_idx = std::clamp(disp_aa_mode, 0, int(out_choices.size()) - 1);
        return;
    }

    if (item_idx == 4) {
        const auto choices = GetAvailableUpscalerChoices();
        int sel = 0;
        for (int i = 0; i < int(choices.size()); ++i) {
            const char *en = nullptr, *ru = nullptr;
            UpscalerChoiceLabels(choices[i], en, ru);
            add_choice(en, ru);
            if (choices[i] == disp_upscaler_choice) sel = i;
        }
        out_selected_idx = sel;
        return;
    }

    if (item_idx == 5) {
        for (int p = int(BbSettings::NativeAA); p <= int(BbSettings::UltraPerformance); ++p) {
            const char *en = nullptr, *ru = nullptr;
            UpscalerModeLabels(p, en, ru);
            add_choice(en, ru);
        }
        out_selected_idx =
            std::clamp(disp_upscale_preset, int(BbSettings::NativeAA), int(BbSettings::UltraPerformance));
        return;
    }

    if (item_idx == 7) {
        add_choice("Off (PS4 Default)", "Выкл (Как на PS4)");
        add_choice("4x (Medium)", "4x (Средняя)");
        add_choice("8x (High)", "8x (Высокая)");
        add_choice("16x (Ultra)", "16x (Ультра)");
        const int aniso = s.anisotropic.load();
        out_selected_idx = aniso >= 16 ? 3 : aniso >= 8 ? 2 : aniso >= 4 ? 1 : 0;
        return;
    }

    if (item_idx == 8) {
        add_choice("Highest", "Максимальная");
        add_choice("Game Default", "Как в игре");
        add_choice("Lower", "Ниже");
        add_choice("Lowest", "Минимальная");
        const int lod = s.model_lod.load();
        out_selected_idx = lod <= -2 ? 0 : lod == 0 ? 1 : lod == 1 ? 2 : 3;
    }
}

void RefreshDisplaySlotTexts(int slot) {
    const int item_idx = std::clamp(display_scroll + slot, 0, TotalDisplayItems - 1);
    const u32 name_id = DisplaySlotName + u32(slot);
    const u32 help_id = DisplaySlotHelp + u32(slot);
    const u32 val_id = DisplaySlotVal + u32(slot);
    const auto& s = BbSettings::Get();

    if (item_idx == 0) {
        SetDynamic(name_id, "Aspect Ratio", "Соотношение сторон");
        SetDynamic(help_id,
                   "Of the final frame: wider sees more to the sides, the UI keeps to the screen edges",
                   "Готового кадра: шире — больше видно по бокам, интерфейс у краёв экрана");
        if (disp_aspect == BbSettings::AspectAuto) {
            const std::string group = BbSettings::AspectKey(BbSettings::ResolvedAspect(BbSettings::AspectAuto));
            SetDynamic(val_id, ("Auto (" + group + ")").c_str(), ("Авто (" + group + ")").c_str());
        } else {
            const char* k = BbSettings::AspectKey(disp_aspect);
            SetDynamic(val_id, k, k);
        }
        return;
    }

    if (item_idx == 1) {
        SetDynamic(name_id, "Resolution", "Разрешение");
        SetDynamic(help_id, "Size of the final frame and UI (applies live without restart)",
                   "Размер готового кадра и интерфейса (применяется сразу без перезапуска)");
        const auto o = MenuOutput(disp_aspect, disp_output_idx);
        const std::string txt = fmt::format("{} x {}", o.width, o.height);
        SetDynamic(val_id, txt.c_str(), txt.c_str());
        return;
    }

    if (item_idx == 2) {
        SetDynamic(name_id, "FPS Limit", "Лимит FPS");
        SetDynamic(help_id,
                   "Maximum frame rate target: Uncapped or fixed FPS (custom value in Insert overlay)",
                   "Максимальная частота кадров: без лимита или точный FPS (свой в меню Insert)");
        const int fps = BbFrameRate::Limit();
        if (fps <= 0) {
            SetDynamic(val_id, "Uncapped", "Без лимита");
        } else {
            bool is_preset = false;
            for (int p : FpsPresets) {
                if (p == fps) is_preset = true;
            }
            const std::string en = is_preset ? fmt::format("{} FPS", fps) : fmt::format("{} FPS (Custom)", fps);
            const std::string ru = is_preset ? fmt::format("{} FPS", fps) : fmt::format("{} FPS (Свой)", fps);
            SetDynamic(val_id, en.c_str(), ru.c_str());
        }
        return;
    }

    if (item_idx == 3) {
        SetDynamic(name_id, "Anti-Aliasing", "Сглаживание");
        SetDynamic(help_id,
                   "Anti-Aliasing mode when Upscaler is Off (Off / FXAA / TAA / DLAA 4 Transformer)",
                   "Режим сглаживания (Выкл / FXAA / TAA / DLAA 4 Трансформер)");
        if (disp_aa_mode == AaOff) {
            SetDynamic(val_id, "Off", "Выкл");
        } else if (disp_aa_mode == AaFxaa) {
            SetDynamic(val_id, "FXAA", "FXAA");
        } else if (disp_aa_mode == AaTaa) {
            SetDynamic(val_id, "TAA", "TAA");
        } else {
            SetDynamic(val_id, "DLAA (DLSS 4)", "DLAA (DLSS 4)");
        }
        return;
    }

    if (item_idx == 4) {
        SetDynamic(name_id, "Upscaler", "Апскейлер");
        SetDynamic(help_id,
                   "Super-resolution upscaler (Off = Native 100%; DLSS 4.5 Preset M/L, DLSS 4 Preset K, FSR 3.1)",
                   "Апскейлер (Выкл = Натив 100%; DLSS 4.5 Preset M/L, DLSS 4 Preset K, FSR 3.1)");
        const char *en = nullptr, *ru = nullptr;
        UpscalerChoiceLabels(disp_upscaler_choice, en, ru);
        SetDynamic(val_id, en, ru);
        return;
    }

    if (item_idx == 5) {
        SetDynamic(name_id, "Upscaler Mode", "Режим апскейлера");
        SetDynamic(help_id,
                   "3D render scale when Upscaler is active: Native 100% (DLAA), Quality (67%), Balanced (58%), Performance (50%), Ultra Perf. (33%)",
                   "Масштаб 3D-рендера: Натив 100% (DLAA), Качество (67%), Баланс (58%), Производ. (50%), Ультра (33%)");
        const char *p_en = nullptr, *p_ru = nullptr;
        UpscalerModeLabels(disp_upscale_preset, p_en, p_ru);
        SetDynamic(val_id, p_en, p_ru);
        return;
    }

    if (item_idx == 6) {
        SetDynamic(name_id, "Sharpness (RCAS)", "Резкость (RCAS)");
        SetDynamic(help_id, "Contrast-adaptive sharpening (RCAS): 0 off, 10 strongest (Left/Right)",
                   "Адаптивная резкость (RCAS): 0 выкл, 10 сильнее всего (Влево/Вправо)");
        const int step = CurrentSharpnessStep();
        const std::string txt = fmt::format("{}", step);
        SetDynamic(val_id, txt.c_str(), txt.c_str());
        return;
    }

    if (item_idx == 7) {
        SetDynamic(name_id, "Anisotropic Filtering", "Фильтрация текстур");
        SetDynamic(help_id,
                   "Sharpens ground, cobblestones and wall textures at oblique angles (applies live)",
                   "Чёткость текстур брусчатки, земли и стен под острым углом (применяется мгновенно)");
        const int aniso = s.anisotropic.load();
        if (aniso >= 16) {
            SetDynamic(val_id, "16x (Ultra)", "16x (Ультра)");
        } else if (aniso >= 8) {
            SetDynamic(val_id, "8x (High)", "8x (Высокая)");
        } else if (aniso >= 4) {
            SetDynamic(val_id, "4x (Medium)", "4x (Средняя)");
        } else {
            SetDynamic(val_id, "Off (PS4 Default)", "Выкл (Как на PS4)");
        }
        return;
    }

    if (item_idx == 8) {
        SetDynamic(name_id, "Model Detail (LOD)", "Детализация моделей");
        SetDynamic(help_id, "Level of detail distance for 3D models and geometry (applies live)",
                   "Дальность максимальной детализации 3D-моделей (применяется мгновенно)");
        const int lod = s.model_lod.load();
        if (lod <= -2) {
            SetDynamic(val_id, "Highest", "Максимальная");
        } else if (lod == 0) {
            SetDynamic(val_id, "Game Default", "Как в игре");
        } else if (lod == 1) {
            SetDynamic(val_id, "Lower", "Ниже");
        } else {
            SetDynamic(val_id, "Lowest", "Минимальная");
        }
    }
}

void UpdateDisplaySlotRowDisplay(u8* row, int slot) {
    if (!row) return;
    const int item_idx = std::clamp(display_scroll + slot, 0, TotalDisplayItems - 1);
    if (item_idx == 6) {
        const std::vector<u32> single = {DisplaySlotVal + u32(slot), DisplaySlotVal + u32(slot)};
        PopulateSelectRowChoices(row, FirstDisplaySlot + slot, single, 0);
    } else {
        std::vector<u32> choice_ids;
        int selected_idx = 0;
        BuildDisplaySlotChoices(slot, choice_ids, selected_idx);
        PopulateSelectRowChoices(row, FirstDisplaySlot + slot, choice_ids, selected_idx);
    }
    const int sel = GetSelectedDisplayRow();
    SyncDisplaySlotSliderWidget(row, slot, slot == sel);
}

void RefreshDisplayPage() {
    display_scroll = std::clamp(display_scroll, 0, MaxDisplayScroll);
    for (int slot = 0; slot < DisplaySlotCount; ++slot) {
        RefreshDisplaySlotTexts(slot);
    }
    if (active_display_page) {
        u8* const rows_begin = *reinterpret_cast<u8**>(active_display_page + 0xe58);
        u8* const rows_end = *reinterpret_cast<u8**>(active_display_page + 0xe60);
        if (rows_begin && rows_end && rows_end - rows_begin >= DisplaySlotCount * 0x90) {
            for (int slot = 0; slot < DisplaySlotCount; ++slot) {
                u8* const entry = rows_begin + slot * 0x90;
                *reinterpret_cast<const char16_t**>(entry + 0x00) = TextOf(DisplaySlotName + u32(slot));
                *reinterpret_cast<const char16_t**>(entry + 0x40) = TextOf(DisplaySlotHelp + u32(slot));
                u8* const widget = *reinterpret_cast<u8**>(entry + 0x80);
                if (widget) {
                    display_slot_rows[slot] = widget;
                }
            }
        }
        Game<void (PS4_SYSV_ABI*)(void*)>(ListDraw)(active_display_page + 0xa70);
        Game<void (PS4_SYSV_ABI*)(void*)>(UpdatePageHelpBar)(active_display_page);
    }
    for (int slot = 0; slot < DisplaySlotCount; ++slot) {
        UpdateDisplaySlotRowDisplay(display_slot_rows[slot], slot);
    }
}

void InitDisplayPage(void* page) {
    active_display_page = static_cast<u8*>(page);
    display_scroll = 0;
    display_wrapped_from_top = false;
    for (int slot = 0; slot < DisplaySlotCount; ++slot) {
        display_slot_rows[slot] = nullptr;
    }
    LoadValues();
    for (int slot = 0; slot < DisplaySlotCount; ++slot) {
        RefreshDisplaySlotTexts(slot);
    }
    AddRows(page, pages[0].rows);
    u8* const rows_begin = *reinterpret_cast<u8**>(active_display_page + 0xe58);
    u8* const rows_end = *reinterpret_cast<u8**>(active_display_page + 0xe60);
    if (rows_begin && rows_end && rows_end - rows_begin >= DisplaySlotCount * 0x90) {
        for (int slot = 0; slot < DisplaySlotCount; ++slot) {
            display_slot_rows[slot] = *reinterpret_cast<u8**>(rows_begin + slot * 0x90 + 0x80);
            UpdateDisplaySlotRowDisplay(display_slot_rows[slot], slot);
        }
    }
}

void ApplyDisplaySlotChoiceIndex(u8* row, int slot, int choice_idx) {
    auto& s = BbSettings::Get();
    const int item_idx = std::clamp(display_scroll + slot, 0, TotalDisplayItems - 1);

    if (item_idx == 0) {
        const int new_aspect = std::clamp(choice_idx, 0, BbSettings::AspectCount - 1);
        if (new_aspect != disp_aspect) {
            const BbSettings::OutputSize shown = MenuOutput(disp_aspect, disp_output_idx);
            disp_aspect = new_aspect;
            menu_outputs = BbSettings::Outputs(disp_aspect);
            disp_output_idx = MenuOutputIndex(disp_aspect, BbSettings::NearestOutput(disp_aspect, shown));
            MarkDisplayHeavyDirty();
        }
        RefreshDisplayPage();
        return;
    }

    if (item_idx == 1) {
        const int n = std::max(1, menu_outputs.count);
        const int new_idx = std::clamp(choice_idx, 0, n - 1);
        if (new_idx != disp_output_idx) {
            disp_output_idx = new_idx;
            MarkDisplayHeavyDirty();
        }
        RefreshDisplayPage();
        return;
    }

    if (item_idx == 2) {
        const std::vector<int> fps_vals = BuildFpsValuesList(nullptr);
        if (!fps_vals.empty()) {
            const int new_fps = fps_vals[std::clamp(choice_idx, 0, int(fps_vals.size()) - 1)];
            if (new_fps != BbFrameRate::Limit()) {
                s.fps_limit = new_fps;
                BbFrameRate::SetLimit(new_fps);
                BbSettings::Save();
            }
        }
        RefreshDisplaySlotTexts(slot);
        UpdateDisplaySlotRowDisplay(row, slot);
        return;
    }

    if (item_idx == 3) {
        std::vector<int> modes = {AaOff, AaFxaa, AaTaa};
        if (s.dlss_supported.load()) {
            modes.push_back(AaDlaa);
        }
        const int new_mode = modes[std::clamp(choice_idx, 0, int(modes.size()) - 1)];
        if (new_mode != disp_aa_mode) {
            disp_aa_mode = new_mode;
            MarkDisplayHeavyDirty();
        }
        RefreshDisplaySlotTexts(slot);
        UpdateDisplaySlotRowDisplay(row, slot);
        return;
    }

    if (item_idx == 4) {
        const auto choices = GetAvailableUpscalerChoices();
        const auto new_up = choices[std::clamp(choice_idx, 0, int(choices.size()) - 1)];
        if (new_up != disp_upscaler_choice) {
            disp_upscaler_choice = new_up;
            MarkDisplayHeavyDirty();
        }
        RefreshDisplaySlotTexts(slot);
        UpdateDisplaySlotRowDisplay(row, slot);
        return;
    }

    if (item_idx == 5) {
        const int new_preset = std::clamp(choice_idx, int(BbSettings::NativeAA), int(BbSettings::UltraPerformance));
        if (new_preset != disp_upscale_preset) {
            disp_upscale_preset = new_preset;
            MarkDisplayHeavyDirty();
        }
        RefreshDisplaySlotTexts(slot);
        UpdateDisplaySlotRowDisplay(row, slot);
        return;
    }

    if (item_idx == 7) {
        const int new_aniso = AnisoValues[std::clamp(choice_idx, 0, AnisoValueCount - 1)];
        if (new_aniso != s.anisotropic.load()) {
            s.anisotropic = new_aniso;
            BbSettings::Save();
        }
        RefreshDisplaySlotTexts(slot);
        UpdateDisplaySlotRowDisplay(row, slot);
        return;
    }

    if (item_idx == 8) {
        const int idx = std::clamp(choice_idx, 0, 3);
        const int new_lod = LodValues[idx];
        if (new_lod != s.model_lod.load()) {
            s.model_lod = new_lod;
            values[ModelLod] = idx;
            applied[ModelLod] = idx;
            BbGuestHooks::ApplyGameEffects();
            BbSettings::Save();
        }
        RefreshDisplaySlotTexts(slot);
        UpdateDisplaySlotRowDisplay(row, slot);
    }
}

void StepDisplaySlotSetting(u8* row, int slot, int delta, bool wrap) {
    const int item_idx = std::clamp(display_scroll + slot, 0, TotalDisplayItems - 1);
    if (item_idx == 6) {
        int step = CurrentSharpnessStep() + delta;
        step = wrap ? (step + SharpnessSteps) % SharpnessSteps : std::clamp(step, 0, SharpnessSteps - 1);
        SetSharpnessStep(step);
        PlayMenuClickSound();
        RefreshDisplaySlotTexts(slot);
        UpdateDisplaySlotRowDisplay(row, slot);
        return;
    }
    std::vector<u32> choice_ids;
    int cur_idx = 0;
    BuildDisplaySlotChoices(slot, choice_ids, cur_idx);
    const int n = std::max(1, int(choice_ids.size()));
    const int next_idx = (cur_idx + delta + n) % n;
    PlayMenuClickSound();
    ApplyDisplaySlotChoiceIndex(row, slot, next_idx);
}

void ShowCapturePrompt(u8* row, int slot, int k, const std::string& captured_text) {
    std::string primary, alt;
    SplitBinding(BbSettings::GetKeyBinding(k), primary, alt);
    std::string en, ru;
    if (!capturing_alt) {
        en = captured_text.empty() ? "[ Press key(s)... ]" : ("[ " + captured_text + " ... ]");
        ru = captured_text.empty() ? "[ Нажмите клавишу... ]" : ("[ " + captured_text + " ... ]");
    } else {
        const std::string a_en =
            captured_text.empty() ? "[ +Alt Key (Bksp=Clear) ]" : ("[ " + captured_text + " ... ]");
        const std::string a_ru =
            captured_text.empty() ? "[ +Альт. кл. (Bksp=Сброс) ]" : ("[ " + captured_text + " ... ]");
        en = BbSettings::FormatKeyBinding(primary, false) + " / " + a_en;
        ru = BbSettings::FormatKeyBinding(primary, true) + " / " + a_ru;
    }
    SetDynamic(ControlsSlotVal + u32(slot), en.c_str(), ru.c_str());
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
            RefreshSlotTexts(slot);
            UpdateSlotRowDisplay(row, slot);
            capture_cooldown_ms.store(NowSteadyMs() + 300, std::memory_order_relaxed);
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
        std::string preview;
        for (std::size_t i = 0; i < captured_tokens.size(); ++i) {
            if (i > 0) preview += " + ";
            preview += captured_tokens[i];
        }
        if (preview != last_capture_preview) {
            last_capture_preview = preview;
            ShowCapturePrompt(row, slot, k, preview);
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
        std::string primary, alt;
        SplitBinding(BbSettings::GetKeyBinding(k), primary, alt);
        if (capturing_alt) {
            if (new_binding == "Backspace" || new_binding == "Delete") {
                alt.clear();
            } else {
                alt = new_binding;
            }
            BbSettings::SetKeyBinding(k, JoinBinding(primary, alt));
        } else {
            BbSettings::SetKeyBinding(k, new_binding);
        }
        BbSettings::Save();
        RefreshSlotTexts(slot);
        UpdateSlotRowDisplay(row, slot);
        PlayMenuClickSound();
        capture_cooldown_ms.store(NowSteadyMs() + 300, std::memory_order_relaxed);
        capturing_action.store(-1, std::memory_order_relaxed);
    }
}

// ---- Hooks ----
using ControlsContentFn = void (PS4_SYSV_ABI*)(void*, u8*);
ControlsContentFn controls_content_original = nullptr;

void PS4_SYSV_ABI ControlsContentHook(void* page, u8* game_settings) {
    active_controls_page = static_cast<u8*>(page);
    active_game_settings = game_settings;
    controls_scroll = 0;
    controls_wrapped_from_top = false;
    capture_cooldown_ms.store(NowSteadyMs() + 250, std::memory_order_relaxed);
    for (int slot = 0; slot < ControlsSlotCount; ++slot) {
        slot_rows[slot] = nullptr;
    }
    LoadValues();
    for (int slot = 0; slot < ControlsSlotCount; ++slot) {
        RefreshSlotTexts(slot);
    }
    AddRows(page, controls_rows);
    u8* const rows_begin = *reinterpret_cast<u8**>(active_controls_page + 0xe58);
    u8* const rows_end = *reinterpret_cast<u8**>(active_controls_page + 0xe60);
    if (rows_begin && rows_end && rows_end - rows_begin >= ControlsSlotCount * 0x90) {
        for (int slot = 0; slot < ControlsSlotCount; ++slot) {
            slot_rows[slot] = *reinterpret_cast<u8**>(rows_begin + slot * 0x90 + 0x80);
            UpdateSlotRowDisplay(slot_rows[slot], slot);
        }
    }
}

using OptionsPageNavigateFn = void (PS4_SYSV_ABI*)(u8*, u8*, float);
OptionsPageNavigateFn options_page_navigate_original = nullptr;

bool AnySlotPopupOpen(u8* const* rows, int slot_count) {
    for (int s = 0; s < slot_count; ++s) {
        if (rows[s] && rows[s][0x3c0] != 0) {
            return true;
        }
    }
    return false;
}

void HandleScrollablePageNavigate(u8* page, u8* input, float frame_time, int slot_count, int max_scroll,
                                  int& scroll, bool& wrapped_from_top, u8* const* rows,
                                  void (*refresh_fn)(), void (*sync_sliders_fn)(int)) {
    const bool popup_was_open = AnySlotPopupOpen(rows, slot_count);
    const int pre_sel = GetSelectedPageRow(page);
    options_page_navigate_original(page, input, frame_time);
    if (popup_was_open || AnySlotPopupOpen(rows, slot_count)) {
        return;
    }
    const int hover_sel = BbMenuPointer::PostHoverCursor(page + 0xa70, pre_sel);
    const int new_sel = GetSelectedPageRow(page);
    if (new_sel != hover_sel) {
        if (hover_sel == slot_count - 1 && new_sel == slot_count && scroll < max_scroll) {
            ++scroll;
            wrapped_from_top = false;
            *reinterpret_cast<int*>(page + 0xae8) = -1;
            Game<int (PS4_SYSV_ABI*)(void*, u32)>(ListSelect)(page + 0xa70, u32(slot_count - 1));
            Game<void (PS4_SYSV_ABI*)(void*)>(ListShow)(page + 0xa70);
            refresh_fn();
            return;
        }
        if (hover_sel == 0 && new_sel == slot_count) {
            if (scroll > 0) {
                --scroll;
                wrapped_from_top = false;
                *reinterpret_cast<int*>(page + 0xae8) = -1;
                Game<int (PS4_SYSV_ABI*)(void*, u32)>(ListSelect)(page + 0xa70, 0);
                Game<void (PS4_SYSV_ABI*)(void*)>(ListShow)(page + 0xa70);
                refresh_fn();
                return;
            }
            wrapped_from_top = true;
        } else if (hover_sel == slot_count && new_sel == slot_count - 1 &&
                   wrapped_from_top && scroll != max_scroll) {
            scroll = max_scroll;
            wrapped_from_top = false;
            refresh_fn();
            return;
        } else if (hover_sel == slot_count && new_sel == 0 && scroll != 0) {
            scroll = 0;
            wrapped_from_top = false;
            refresh_fn();
            return;
        } else {
            wrapped_from_top = false;
        }
    } else if (new_sel != pre_sel) {
        wrapped_from_top = false;
    }
    if (new_sel != pre_sel || new_sel != hover_sel) {
        sync_sliders_fn(new_sel);
    }
}

void SyncAllControlsSliders(int new_sel) {
    for (int slot = 0; slot < ControlsSlotCount; ++slot) {
        SyncSlotSliderWidget(slot_rows[slot], slot, slot == new_sel);
    }
}

void SyncAllDisplaySliders(int new_sel) {
    for (int slot = 0; slot < DisplaySlotCount; ++slot) {
        SyncDisplaySlotSliderWidget(display_slot_rows[slot], slot, slot == new_sel);
    }
}

void PS4_SYSV_ABI OptionsPageNavigateHook(u8* page, u8* input, float frame_time) {
    if (page == active_controls_page) {
        if (capturing_action.load(std::memory_order_relaxed) >= 0 || InCaptureCooldown()) {
            return;
        }
        HandleScrollablePageNavigate(page, input, frame_time, ControlsSlotCount, MaxControlsScroll,
                                     controls_scroll, controls_wrapped_from_top, slot_rows,
                                     &RefreshControlsPage, &SyncAllControlsSliders);
        return;
    }
    if (page == active_display_page) {
        HandleScrollablePageNavigate(page, input, frame_time, DisplaySlotCount, MaxDisplayScroll,
                                     display_scroll, display_wrapped_from_top, display_slot_rows,
                                     &RefreshDisplayPage, &SyncAllDisplaySliders);
        return;
    }
    options_page_navigate_original(page, input, frame_time);
}

using RowInputFn = u64 (PS4_SYSV_ABI*)(u8*, u64, u64, u64, u64, u64, double);
RowInputFn row_input_original = nullptr;

u64 PS4_SYSV_ABI RowInputHook(u8* row, u64 a1, u64 a2, u64 a3, u64 a4, u64 a5, double frame_time) {
    int* const val_ptr = *reinterpret_cast<int**>(row + RowValue);
    // The game asks the open page's rows: the resolution row is alive while it does.
    if (val_ptr == &values[OutputRes]) {
        RefreshOutputRow(row);
    }
    if (val_ptr >= &values[FirstDisplaySlot] &&
        val_ptr < &values[int(FirstDisplaySlot) + DisplaySlotCount]) {
        const int slot = int(val_ptr - &values[FirstDisplaySlot]);
        display_slot_rows[slot] = row;
        if (values[FirstDisplaySlot + slot] == DefaultResetSentinel) {
            auto& s = BbSettings::Get();
            disp_aspect = BbSettings::AspectAuto;
            menu_outputs = BbSettings::Outputs(disp_aspect);
            disp_output_idx =
                MenuOutputIndex(disp_aspect, BbSettings::NearestOutput(disp_aspect, BbSettings::Monitor()));
            disp_upscaler_choice = DispUpOff;
            disp_upscale_preset = BbSettings::Quality;
            disp_aa_mode = s.dlss_supported.load() ? AaDlaa : AaTaa;
            MarkDisplayHeavyDirty();
            s.fps_limit = 0;
            BbFrameRate::SetLimit(0);
            SetSharpnessStep(2);
            s.anisotropic = 16;
            s.model_lod = 0;
            BbSettings::Save();
            RefreshDisplayPage();
            return 0;
        }
        const int item_idx = std::clamp(display_scroll + slot, 0, TotalDisplayItems - 1);
        if (item_idx == 6) {
            // Sharpness slider row: Left/Right/Confirm step the slider.
            const u8 in = u8(a1);
            if (in != 0) {
                const bool left = Game<bool (PS4_SYSV_ABI*)(const u8*)>(IsLeftPressed)(&in);
                const bool right = !left && Game<bool (PS4_SYSV_ABI*)(const u8*)>(IsRightPressed)(&in);
                const bool confirm = !left && !right && Game<bool (PS4_SYSV_ABI*)(const u8*)>(IsConfirmPressed)(&in);
                if (left || right || confirm) {
                    StepDisplaySlotSetting(row, slot, left ? -1 : 1, confirm);
                }
            }
            return 0;
        }
        // Multi-choice Display row: supports both native pop-up dropdown (Confirm) and quick Left/Right step.
        if (row[0x3c0] != 0) {
            const u64 res = row_input_original(row, a1, a2, a3, a4, a5, frame_time);
            if (row[0x3c0] == 0) {
                ApplyDisplaySlotChoiceIndex(row, slot, values[FirstDisplaySlot + slot]);
            }
            return res;
        }
        const u8 in = u8(a1);
        if (in != 0) {
            const bool left = Game<bool (PS4_SYSV_ABI*)(const u8*)>(IsLeftPressed)(&in);
            const bool right = !left && Game<bool (PS4_SYSV_ABI*)(const u8*)>(IsRightPressed)(&in);
            if (left || right) {
                StepDisplaySlotSetting(row, slot, left ? -1 : 1, false);
                return 0;
            }
        }
        return row_input_original(row, a1, a2, a3, a4, a5, frame_time);
    }
    if (val_ptr >= &values[FirstSlot] && val_ptr < &values[int(FirstSlot) + ControlsSlotCount]) {
        const int slot = int(val_ptr - &values[FirstSlot]);
        slot_rows[slot] = row;
        // If the page's "Defaults" button was pressed, it sets values[FirstSlot + slot] = DefaultResetSentinel.
        if (values[FirstSlot + slot] == DefaultResetSentinel) {
            for (int k = 0; k < BbSettings::KeyActionCount; ++k) {
                BbSettings::SetKeyBinding(k, BbSettings::GetKeyActionDef(k).default_val);
            }
            SetMouseSpeedStep(5);
            if (active_game_settings) {
                active_game_settings[0x0] = 5;  // Camera Sensitivity
                active_game_settings[0x1] = 10; // Controller Vibration
                active_game_settings[0x9] = 1;  // Camera Y-Axis (1 = Normal)
                active_game_settings[0xa] = 1;  // Camera X-Axis (1 = Normal)
                active_game_settings[0xb] = 1;  // Auto Target Switch (1 = On)
                active_game_settings[0xc] = 1;  // Camera Auto-Adjust (1 = On)
            }
            RefreshControlsPage();
            return 0;
        }
        const int item_idx = std::clamp(controls_scroll + slot, 0, TotalControlsItems - 1);
        const int k = ItemKeyAction(item_idx);
        if (k >= 0) {
            const int active_k = capturing_action.load(std::memory_order_relaxed);
            if (active_k == k && capturing_slot == slot) {
                StepKeyCapture(row, slot, k);
                return 0;
            }
            if (active_k < 0 && !InCaptureCooldown()) {
                const u8 in = u8(a1);
                if (in != 0) {
                    const bool right = Game<bool (PS4_SYSV_ABI*)(const u8*)>(IsRightPressed)(&in);
                    const bool confirm =
                        !right && Game<bool (PS4_SYSV_ABI*)(const u8*)>(IsConfirmPressed)(&in) &&
                        !IsBareKeyboardF();
                    if (right || confirm) {
                        PlayMenuClickSound();
                        captured_tokens.clear();
                        last_capture_preview.clear();
                        capture_phase = CapturePhase::WaitInitialRelease;
                        last_capture_step_ms.store(NowSteadyMs(), std::memory_order_relaxed);
                        capturing_slot = slot;
                        capturing_alt = right;
                        capturing_action.store(k, std::memory_order_relaxed);
                        ShowCapturePrompt(row, slot, k, {});
                    }
                }
            }
            return 0;
        }
        if (item_idx >= 13 && item_idx <= 15) {
            // Slider row (Mouse Speed, Camera Sensitivity, Controller Vibration)
            const u8 in = u8(a1);
            if (in != 0 && capturing_action.load(std::memory_order_relaxed) < 0 && !InCaptureCooldown()) {
                const bool left = Game<bool (PS4_SYSV_ABI*)(const u8*)>(IsLeftPressed)(&in);
                const bool right = !left && Game<bool (PS4_SYSV_ABI*)(const u8*)>(IsRightPressed)(&in);
                const bool confirm = !left && !right && Game<bool (PS4_SYSV_ABI*)(const u8*)>(IsConfirmPressed)(&in);
                if (left || right || confirm) {
                    StepSlotSetting(row, slot, left ? -1 : 1, confirm);
                }
            }
            return 0;
        }
        // Camera & Target multi-choice row (item_idx 16..19): supports both pop-up dropdown and Left/Right step.
        if (row[0x3c0] != 0) {
            const u64 res = row_input_original(row, a1, a2, a3, a4, a5, frame_time);
            if (row[0x3c0] == 0) {
                ApplyControlsSlotChoiceIndex(row, slot, values[FirstSlot + slot]);
            }
            return res;
        }
        const u8 in = u8(a1);
        if (in != 0 && capturing_action.load(std::memory_order_relaxed) < 0 && !InCaptureCooldown()) {
            const bool left = Game<bool (PS4_SYSV_ABI*)(const u8*)>(IsLeftPressed)(&in);
            const bool right = !left && Game<bool (PS4_SYSV_ABI*)(const u8*)>(IsRightPressed)(&in);
            if (left || right) {
                StepSlotSetting(row, slot, left ? -1 : 1, false);
                return 0;
            }
        }
        return row_input_original(row, a1, a2, a3, a4, a5, frame_time);
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
    for (int slot = 0; slot < ControlsSlotCount; ++slot) {
        default_values[FirstSlot + slot] = DefaultResetSentinel; // triggers default reset when "Defaults" is clicked
    }
    for (int slot = 0; slot < DisplaySlotCount; ++slot) {
        default_values[FirstDisplaySlot + slot] = DefaultResetSentinel; // triggers default reset on Display page
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
        Detour(image, OptionsPageNavigate, AddPageItemPrologue, sizeof(AddPageItemPrologue),
               reinterpret_cast<void*>(&OptionsPageNavigateHook),
               reinterpret_cast<void**>(&options_page_navigate_original), cursor) &&
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
    return false;
}

bool InCaptureCooldown() {
    return NowSteadyMs() < capture_cooldown_ms.load(std::memory_order_relaxed);
}

void Poll() {
    if (!values_valid) {
        return;
    }
    auto& s = BbSettings::Get();
    bool saved = false;

    // Apply debounced Display changes (Aspect Ratio, Resolution, Anti-Aliasing, Upscaler, Upscaler Mode)
    // once the user has rested for 100 ms, so stepping through choices feels instant without double-recreating.
    if (display_heavy_dirty.load(std::memory_order_acquire) &&
        NowSteadyMs() - display_heavy_dirty_ms.load(std::memory_order_relaxed) >= 100) {
        display_heavy_dirty.store(false, std::memory_order_release);
        const int aspect = std::clamp(disp_aspect, 0, BbSettings::AspectCount - 1);
        s.output_res = BbSettings::PackSize(MenuOutput(aspect, disp_output_idx));
        s.aspect = aspect;

        int target_upscaler = BbSettings::UpscalerOff;
        int target_dlss_preset = 11;
        int target_preset = BbSettings::NativeAA;
        bool target_fxaa = false;

        if (disp_upscaler_choice == DispUpOff) {
            target_preset = BbSettings::NativeAA;
            if (disp_aa_mode == AaOff) {
                target_upscaler = BbSettings::UpscalerOff;
                target_fxaa = false;
            } else if (disp_aa_mode == AaFxaa) {
                target_upscaler = BbSettings::UpscalerOff;
                target_fxaa = true;
            } else if (disp_aa_mode == AaTaa) {
                target_upscaler = BbSettings::UpscalerTaa;
            } else {
                target_upscaler = s.dlss_supported.load() ? BbSettings::UpscalerDlss : BbSettings::UpscalerTaa;
                target_dlss_preset = 11;
            }
        } else {
            target_preset = std::clamp(disp_upscale_preset, int(BbSettings::NativeAA), int(BbSettings::UltraPerformance));
            target_fxaa = (disp_aa_mode == AaFxaa);
            switch (disp_upscaler_choice) {
            case DispUpDlss4:
                target_upscaler = BbSettings::UpscalerDlss;
                target_dlss_preset = 11;
                break;
            case DispUpDlss45M:
                target_upscaler = BbSettings::UpscalerDlss;
                target_dlss_preset = 13;
                break;
            case DispUpDlss45L:
                target_upscaler = BbSettings::UpscalerDlss;
                target_dlss_preset = 12;
                break;
            case DispUpDlss3:
                target_upscaler = BbSettings::UpscalerDlss;
                target_dlss_preset = 0;
                break;
            case DispUpFsr4:
                target_upscaler = BbSettings::UpscalerFsr4;
                break;
            case DispUpFsr411:
                target_upscaler = BbSettings::UpscalerFsr411;
                break;
            case DispUpFsr31:
            default:
                target_upscaler = BbSettings::UpscalerFsr3;
                break;
            }
        }

        s.upscaler = UpscalerAvailable(target_upscaler) ? target_upscaler : int(BbSettings::UpscalerFsr3);
        s.dlss_preset = target_dlss_preset;
        s.preset = target_preset;
        s.effects[GameAaEffectIdx] = target_fxaa;
        values[FirstEffect + GameAaEffectIdx] = target_fxaa ? 1 : 0;
        applied[FirstEffect + GameAaEffectIdx] = target_fxaa ? 1 : 0;
        bytes[FirstEffect + GameAaEffectIdx] = u8(target_fxaa ? 1 : 0);
        BbGuestHooks::ApplyGameEffects();
        saved = true;
    }

    // Apply Effects page toggles immediately.
    int now_values[FieldCount];
    std::memcpy(now_values, values, sizeof(now_values));
    for (int f = 0; f < FieldCount; ++f) {
        if (byte_field[f]) {
            now_values[f] = bytes[f];
        }
    }
    bool effects_changed = false;
    for (int e = 0; e < BbSettings::EffectCount; ++e) {
        if (now_values[FirstEffect + e] != applied[FirstEffect + e]) {
            s.effects[e] = now_values[FirstEffect + e] != 0;
            applied[FirstEffect + e] = now_values[FirstEffect + e];
            if (e == GameAaEffectIdx && disp_upscaler_choice == DispUpOff &&
                (disp_aa_mode == AaOff || disp_aa_mode == AaFxaa)) {
                disp_aa_mode = s.effects[e].load() ? AaFxaa : AaOff;
            }
            effects_changed = true;
        }
    }
    if (effects_changed) {
        BbGuestHooks::ApplyGameEffects();
        saved = true;
    }
    if (saved) {
        BbSettings::Save();
    }
}

} // namespace BbGameMenu
