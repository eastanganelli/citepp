#!/usr/bin/env python3
"""
citepp task runner. Stdlib only. Cross-platform (Windows/Linux/macOS).

Commands:
  run-debug        Build Vite (development) + C++ (Debug), then run.
  run-release      Build Vite (production) + C++ (Release), then run.
  compile-release  Build release, package a shareable zip artifact,
                   and optionally tag + publish a GitHub release.

Windows: run from an MSYS2 UCRT64 shell (g++, cmake, ninja on PATH).
"""
import argparse
import json
import platform
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent
FRONTEND = ROOT / "frontend"
IS_WIN = platform.system() == "Windows"
EXE = "app.exe" if IS_WIN else "app"
OS_TAG = {"Windows": "windows", "Linux": "linux", "Darwin": "macos"}[platform.system()]


# ---------------------------------------------------------------- helpers ----
def tool(name: str) -> str:
    p = shutil.which(name)
    if not p:
        sys.exit(f"error: '{name}' not found on PATH. See README > Prerequisites.")
    return p


def run(cmd, cwd=None, check=True):
    printable = " ".join(str(c) for c in cmd)
    print(f"\n$ {printable}\n", flush=True)
    r = subprocess.run([str(c) for c in cmd], cwd=str(cwd) if cwd else None)
    if check and r.returncode != 0:
        sys.exit(f"error: command failed ({r.returncode}): {printable}")
    return r.returncode


def preflight():
    for t in ("cmake", "ninja", "npm"):
        tool(t)
    if IS_WIN:
        if not shutil.which("g++"):
            sys.exit(
                "error: g++ (MinGW-w64) not found.\n"
                "Open the 'MSYS2 UCRT64' shell and install:\n"
                "  pacman -S mingw-w64-ucrt-x86_64-gcc "
                "mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja"
            )


def version() -> str:
    pj = json.loads((FRONTEND / "package.json").read_text())
    return pj.get("version", "0.0.0")


# ------------------------------------------------------------- build steps ----
def build_frontend(debug: bool):
    script = "build:debug" if debug else "build"
    if not (FRONTEND / "node_modules").exists():
        run([tool("npm"), "install"], cwd=FRONTEND)
    run([tool("npm"), "run", script], cwd=FRONTEND)


def build_cpp(config: str) -> Path:
    """config: 'Debug' or 'Release'. Returns the build dir."""
    build_dir = ROOT / f"build-{config.lower()}"
    configure = [
        tool("cmake"), "-S", ROOT, "-B", build_dir,
        "-G", "Ninja", f"-DCMAKE_BUILD_TYPE={config}",
    ]
    if IS_WIN:
        configure += ["-DCMAKE_CXX_COMPILER=g++"]
    run(configure)
    run([tool("cmake"), "--build", build_dir])
    return build_dir


def run_app(build_dir: Path):
    exe = build_dir / EXE
    if not exe.exists():
        sys.exit(f"error: binary not found: {exe}")
    # cwd = build_dir so the app resolves ui/index.html relatively
    run([exe], cwd=build_dir)


# --------------------------------------------------------------- packaging ----
def package(build_dir: Path) -> Path:
    ver = version()
    art_dir = ROOT / "artifacts"
    art_dir.mkdir(exist_ok=True)
    zpath = art_dir / f"citepp-{ver}-{OS_TAG}.zip"
    ui = build_dir / "ui"
    if not (build_dir / EXE).exists():
        sys.exit(f"error: binary missing, run a release build first: {build_dir / EXE}")
    with zipfile.ZipFile(zpath, "w", zipfile.ZIP_DEFLATED) as z:
        z.write(build_dir / EXE, EXE)
        for p in ui.rglob("*"):
            if p.is_file():
                z.write(p, str(Path("ui") / p.relative_to(ui)))
        for extra in ("README.md", "LICENSE"):
            if (ROOT / extra).exists():
                z.write(ROOT / extra, extra)
    print(f"\nartifact: {zpath}  ({zpath.stat().st_size // 1024} KB)")
    return zpath


def make_release(zpath: Path, publish: bool):
    ver = version()
    tag = f"v{ver}"
    if shutil.which("git"):
        # soft tag: don't abort the whole run if the tag already exists
        rc = run([tool("git"), "tag", "-a", tag, "-m", f"release {tag}"], check=False)
        if rc == 0:
            print(f"created git tag {tag} (push with: git push origin {tag})")
        else:
            print(f"note: git tag {tag} already exists or could not be created; continuing")
    else:
        print("note: git not found, skipping tag")

    if publish:
        if shutil.which("gh"):
            run([tool("gh"), "release", "create", tag, zpath,
                 "-t", tag, "-n", f"Release {tag}"], check=False)
        else:
            print("note: gh CLI not found, skipping GitHub release. "
                  f"Publish manually with:\n  gh release create {tag} {zpath}")


# ------------------------------------------------------------------ commands --
def cmd_run_debug(_):
    preflight()
    build_frontend(debug=True)
    run_app(build_cpp("Debug"))


def cmd_run_release(_):
    preflight()
    build_frontend(debug=False)
    run_app(build_cpp("Release"))


def cmd_compile_release(args):
    preflight()
    build_frontend(debug=False)
    build_dir = build_cpp("Release")
    zpath = package(build_dir)
    if args.release:
        make_release(zpath, publish=args.publish)


def main():
    p = argparse.ArgumentParser(description="citepp task runner")
    sub = p.add_subparsers(dest="cmd", required=True)

    sub.add_parser("run-debug", help="build (Vite dev + C++ Debug) and run")
    sub.add_parser("run-release", help="build (Vite prod + C++ Release) and run")

    cr = sub.add_parser("compile-release",
                        help="build release, package artifact zip")
    cr.add_argument("--release", action="store_true",
                    help="also create a git tag vX.Y.Z")
    cr.add_argument("--publish", action="store_true",
                    help="with --release, publish a GitHub release via gh CLI")

    args = p.parse_args()
    {
        "run-debug": cmd_run_debug,
        "run-release": cmd_run_release,
        "compile-release": cmd_compile_release,
    }[args.cmd](args)


if __name__ == "__main__":
    main()
