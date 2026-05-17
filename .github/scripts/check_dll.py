#!/usr/bin/env python3
"""Verify Win32 DLL exports and imports.

Usage: check_dll.py <dll> <export>... [-- <dll> <export>... ...]

Checks that each DLL has the required exports and only imports from
KERNEL32.dll and msvcrt.dll.
"""

import sys
import os
import pefile

ALLOWED_IMPORTS = {"KERNEL32.dll", "msvcrt.dll"}


def check_dll(dll_path, required_exports):
    name = os.path.basename(dll_path)
    failed = False

    pe = pefile.PE(dll_path)

    exports = set()
    if hasattr(pe, "DIRECTORY_ENTRY_EXPORT"):
        for exp in pe.DIRECTORY_ENTRY_EXPORT.symbols:
            if exp.name:
                exports.add(exp.name.decode())

    imports = set()
    if hasattr(pe, "DIRECTORY_ENTRY_IMPORT"):
        for entry in pe.DIRECTORY_ENTRY_IMPORT:
            imports.add(entry.dll.decode())

    pe.close()

    print(f"--- {name} ---")
    print(f"  Exports: {sorted(exports)}")
    print(f"  Imports: {sorted(imports)}")

    for sym in required_exports:
        if sym not in exports:
            print(f"::error::{name}: missing required export: {sym}")
            failed = True

    for imp in imports:
        if imp not in ALLOWED_IMPORTS:
            print(f"::error::{name}: unexpected DLL import: {imp}")
            failed = True

    return failed


def main():
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        sys.exit(1)

    groups = []
    current = []
    for arg in args:
        if arg == "--":
            if current:
                groups.append(current)
            current = []
        else:
            current.append(arg)
    if current:
        groups.append(current)

    failed = False
    for group in groups:
        dll_path = group[0]
        required_exports = group[1:]
        if check_dll(dll_path, required_exports):
            failed = True

    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
