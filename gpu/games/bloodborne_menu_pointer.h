// SPDX-License-Identifier: GPL-2.0-or-later
// Bloodborne (1.09): the mouse as a pointer in the game's own menus (Scaleform lists: the title
// menu, the inventory, the shops, the options). The item under the pointer gets the game's cursor,
// a click presses the confirm button on it (or the slider's arrow it is on), the right button goes
// back, the wheel moves up and down. bloodborne_menu_pointer.cpp has the reverse engineering.
#pragma once

namespace BbMenuPointer {

/// Hooks the game's list update and registers the window and pad parts (bbgpu_add_*). Nothing
/// happens when the game's bytes differ (another build).
void Install();

/// Returns the cursor index of `list` immediately after mouse-hover processing in OnListUpdate
/// (or `fallback` if `list` was not the last updated list).
int PostHoverCursor(const void* list, int fallback);

/// Current pointer X position in 1920x1080 stage coordinates.
float PointerX();

} // namespace BbMenuPointer
