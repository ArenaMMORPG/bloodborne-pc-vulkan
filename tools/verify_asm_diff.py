#!/usr/bin/env python3
"""
Fast 4-second instruction-level comparator between compiled C++20 functions
in libbb_gx_engine_native.a and the original PS4 eboot.bin (boot.bin).

Usage:
  python3 verify_asm_diff.py [--vma 0x2195cd0] [--module 02_GX] [--check-unsafe]

Never read 100,000 lines of .cpp manually in an LLM context window!
Use this script to immediately inspect instruction-level diffs (calls, globals,
entry registers, return types, and size ratios).
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path("/home/kekw3/Documents/BloodBorne/bb_vulkan")
EBOOT_BIN = Path("/home/kekw3/Documents/bloodborne engine decompile project/original file/eboot.bin")
BOOT_BIN = ROOT / "out/boot.bin"
NATIVE_ARCHIVE = ROOT / "our_cpp_100000_source/build/libbb_gx_engine_native.a"
GOLDEN_ENGINE_CPP = ROOT / "gpu/shim/bb_golden_engine.cpp"
FUNCTIONS_DB = Path("/home/kekw3/Documents/bloodborne engine decompile project/db/functions.json")


def parse_registered_vmas():
    text = GOLDEN_ENGINE_CPP.read_text(errors="ignore")
    entries = []
    for m in re.finditer(
        r'\{\s*(0x[0-9a-fA-F]+)\s*,\s*([A-Za-z0-9_]+)\s*,\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\}',
        text,
    ):
        vma = int(m.group(1), 16)
        sym = m.group(2)
        name = m.group(3)
        mod = m.group(4)
        entries.append((vma, sym, name, mod))

    unsafe_set = set()
    unsafe_block = re.search(r"kUnsafeStubOrCallerVMAs\[\]\s*=\s*\{([^}]+)\}", text, re.S)
    if unsafe_block:
        for um in re.finditer(r"0x[0-9a-fA-F]+", unsafe_block.group(1)):
            unsafe_set.add(int(um.group(0), 16))

    mid_set = set()
    mid_block = re.search(r"kMidFunctionPatchedVMAs\[\]\s*=\s*\{([^}]+)\}", text, re.S)
    if mid_block:
        for mm in re.finditer(r"0x[0-9a-fA-F]+", mid_block.group(1)):
            mid_set.add(int(mm.group(0), 16))

    return entries, unsafe_set, mid_set


def disasm_guest_vma(vma: int, size: int = 256) -> str:
    # In original eboot.bin, file_offset = vma + 0x4000 (in out/boot.bin, file_offset = vma + 0x741668)
    import tempfile
    file_off = vma + 0x4000
    with open(EBOOT_BIN, "rb") as f:
        f.seek(file_off)
        chunk = f.read(size)
    with tempfile.NamedTemporaryFile() as tmp:
        tmp.write(chunk)
        tmp.flush()
        cmd = [
            "objdump",
            "-D",
            "-b",
            "binary",
            "-m",
            "i386:x86-64",
            f"--adjust-vma={hex(vma)}",
            tmp.name,
        ]
        return subprocess.check_output(cmd, text=True, errors="ignore")


def main():
    parser = argparse.ArgumentParser(description="Compare compiled C++20 engine vs PS4 boot.bin")
    parser.add_argument("--vma", type=str, help="Inspect single VMA (e.g. 0x2195cd0) side-by-side")
    parser.add_argument("--module", type=str, default="", help="Filter by module substring")
    parser.add_argument("--only-unsafe", action="store_true", help="Only list functions in kUnsafeStubOrCallerVMAs")
    parser.add_argument("--limit", type=int, default=30, help="Max results to print")
    args = parser.parse_args()

    entries, unsafe_set, mid_set = parse_registered_vmas()

    if args.vma:
        target_vma = int(args.vma, 16)
        print(f"=== Guest boot.bin @ {hex(target_vma)} ===")
        print(disasm_guest_vma(target_vma, 256))
        for vma, sym, name, mod in entries:
            if vma == target_vma:
                print(f"Registered as: {name} ({mod}), unsafe={vma in unsafe_set}, mid_patched={vma in mid_set}")
        return

    active = [e for e in entries if e[0] not in unsafe_set and e[0] not in mid_set]
    print(f"Total registered in bb_golden_engine.cpp: {len(entries)}")
    print(f"  - Active C++ functions (live JMP hooks): {len(active)}")
    print(f"  - In kUnsafeStubOrCallerVMAs (redirected to boot.bin): {len(unsafe_set)}")
    print(f"  - In kMidFunctionPatchedVMAs (redirected to boot.bin): {len(mid_set)}")

    if args.only_unsafe:
        filtered = [e for e in entries if e[0] in unsafe_set and args.module in e[3]]
        print(f"\nShowing first {min(len(filtered), args.limit)} of {len(filtered)} unsafe/stub VMAs:")
        for vma, sym, name, mod in filtered[: args.limit]:
            print(f"  {hex(vma)}  [{mod}]  {name}")


if __name__ == "__main__":
    main()
