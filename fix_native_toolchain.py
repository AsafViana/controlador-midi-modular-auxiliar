"""
PlatformIO pre-script to use MSYS2 MinGW64 toolchain for native builds.
The default MinGW GCC 6.3.0 has issues on this system.
"""
Import("env")

import os
import sys

if env.get("PIOPLATFORM") == "native":
    msys2_bin = r"C:\msys64\mingw64\bin"
    if os.path.isdir(msys2_bin):
        env.Replace(
            CC=os.path.join(msys2_bin, "gcc.exe"),
            CXX=os.path.join(msys2_bin, "g++.exe"),
            AR=os.path.join(msys2_bin, "ar.exe"),
            RANLIB=os.path.join(msys2_bin, "ranlib.exe"),
        )
        # Prepend to PATH for build and runtime (DLL resolution)
        current_path = env["ENV"].get("PATH", os.environ.get("PATH", ""))
        env["ENV"]["PATH"] = msys2_bin + os.pathsep + current_path

        # Also add static linking flags to avoid DLL dependencies at runtime
        env.Append(LINKFLAGS=["-static"])
