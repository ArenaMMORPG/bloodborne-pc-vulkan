---
name: Bug Report / Сообщение о баге
about: Report a crash, visual glitch, or performance issue / Сообщить о вылете, графическом баге или просадке FPS
title: "[BUG] "
labels: bug
assignees: ''
---

## System Info / Конфигурация ПК
- **OS / Дистрибутив Linux:** (e.g. Arch Linux / CachyOS / Ubuntu 24.04 / SteamOS)
- **GPU & Driver:** (e.g. RTX 4090 NVIDIA 565.xx / RX 7900 XTX Mesa RADV 24.3)
- **CPU:** (e.g. Ryzen 9 7950X / Core i7-13700K)
- **Game Dump Version:** (e.g. `CUSA03173` GOTY v1.09 / `CUSA00900` v1.09)
- **Resolution & Upscaler:** (e.g. `2560x1440`, Upscaler: `Off` / `FSR 3.1` / `FSR 4` / `TAA`)

## Description / Описание проблемы
Describe what happened and in which location (e.g. Hunter's Dream, Central Yharnam, Clinic).
Опишите, что произошло и в какой локации.

## Isolation Test (`commands.txt`) / Результат быстрой проверки
If this is a visual glitch, which test from `README.md` fixes it?
- [ ] `BB_HW_OCCLUSION_CULL=0 BB_MICRO_SHADOW_CULL=0` (Test 1 — Occlusion Culler)
- [ ] `BB_NATIVE_SHADERS=0 BB_NATIVE_SHADER_MIN=999` (Test 2 — Native GLSL Shaders)
- [ ] `BB_GX_MODULES=01,02` (Test 3 — C++20 Engine Modules 03..07)

## Logs & Screenshots / Логи и скриншоты
Attach the latest log file from `logs/YYYYMMDD_HHMMSS.log` and a screenshot if applicable.
Прикрепите последний лог из папки `logs/` и скриншот.
