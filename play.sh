#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")"

# Auto-detect unpacked PS4 Bloodborne game dump (CUSA03173 / CUSA00900 / CUSA00207 / CUSA01363)
if [[ -z ${BB_GAME_DIR:-} ]]; then
    for cand in ../CUSA03173 ../CUSA00900 ../CUSA00207 ../CUSA01363 ./CUSA03173 ./CUSA00900; do
        if [[ -f "$cand/eboot.bin" ]]; then
            export BB_GAME_DIR="$(realpath "$cand")"
            break
        fi
    done
fi

export BB_PREBUILT="${BB_PREBUILT:-1}"
export BB_DATA_DIR="${BB_DATA_DIR:-$PWD}"
export BB_SAVE_LOG="${BB_SAVE_LOG:-1}"
export BB_GX_MODULES="${BB_GX_MODULES:-all}"
export BB_FPS="${BB_FPS:-uncap}"
export BB_FPS_LIMIT="${BB_FPS_LIMIT:-0}"
export BB_PC_MODEL="${BB_PC_MODEL:-0}"
export BB_PREUPLOAD="${BB_PREUPLOAD:-2}"
export BB_READBACKS="${BB_READBACKS:-0}"
export BB_BREADCRUMBS="${BB_BREADCRUMBS:-0}"
export BB_INDIRECT_GUARD="${BB_INDIRECT_GUARD:-0}"
export BB_OBJECT_MOTION="${BB_OBJECT_MOTION:-0}"
export BB_LIVE_RES="${BB_LIVE_RES:-0}"
export BB_FULLSCREEN="${BB_FULLSCREEN:-1}"
export BB_PRESENT_MODE="${BB_PRESENT_MODE:-Mailbox}"
export BB_SKIP_NETWORK_CHOICE="${BB_SKIP_NETWORK_CHOICE:-1}"
export BB_LANGUAGE="${BB_LANGUAGE:-1}"

exec bash ./run.sh "$@"
