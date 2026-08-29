# cite++

**Native cross-platform desktop apps with a C++ backend and a React + Vite UI.**
No Electron, no Node at runtime — one small binary that renders your React app in
the operating system's built-in webview.

---

## What it is

`cite++` (repo: `citepp`) is a GitHub **template repository** for building desktop
applications where:

- the **UI** is a modern **React + Vite + TypeScript** app,
- the **backend / heavy logic** is **C++**,
- the two talk through [webview/webview]: C++ functions are *bound* into JavaScript,
  and React simply `await`s them.

```
[React UI]  --await window.countPrimes(n)-->  [C++ backend]
  (webview)   <----- { primes, ms } JSON ------  (main.cpp)
```

The name is a play on **Vite** + **C++** ("cite" + "++").

## What it's for

Use it when you want native desktop performance and a real systems language for the
backend (file I/O, number crunching, hardware access, existing C++ libraries) but a
fast, familiar web stack for the interface.

Good fits: internal tools, data/report generators, engineering utilities, anything
that would be heavy or slow in pure JavaScript. The included demo is intentionally
minimal — it shows the two communication directions and nothing else:

- **`countPrimes(n)`** — React sends an argument, C++ computes, returns the result.
- **`systemInfo()`** — React asks with no arguments, C++ returns data.

Why not Electron: Electron bundles a full Chromium + Node runtime (~100 MB, high RAM).
`cite++` uses the OS webview instead:

| OS      | Engine     | Notes                                            |
|---------|------------|--------------------------------------------------|
| Windows | WebView2   | Runtime preinstalled on Windows 10/11            |
| Linux   | WebKitGTK  | `libgtk-3-dev libwebkit2gtk-4.1-dev`             |
| macOS   | WKWebView  | Xcode command line tools                         |

Result: one binary, a few MB instead of a hundred.

## Prerequisites

- **CMake** >= 3.19, **Ninja**, a **C++17** compiler
- **Node.js** 18+ (for building the frontend) and **Python** 3.8+ (task runner)
- **Windows: MinGW-w64 only.** MSVC is intentionally rejected by CMake.
  Install via [MSYS2] and work from the **UCRT64** shell:

  ```bash
  pacman -S mingw-w64-ucrt-x86_64-gcc \
            mingw-w64-ucrt-x86_64-cmake \
            mingw-w64-ucrt-x86_64-ninja
  ```

  > On Windows, run every command below from the **"MSYS2 UCRT64"** terminal so
  > `g++`, `cmake` and `ninja` are on `PATH`.

## Use this template

Click **"Use this template"** on GitHub, or:

```bash
gh repo create my-app --template <your-user>/citepp
```

## How it's used (project layout)

```
tasks.py             Python task runner (build / run / package)
src/main.cpp         C++ backend: the JS bindings (2 demo functions)
frontend/            React + Vite + TypeScript
  src/bridge.ts      typed wrapper over the C++ binds (single source of truth)
  src/App.tsx        demo UI: prime counter + system info
CMakeLists.txt       build config; forbids MSVC, MinGW-only on Windows
CMakePresets.json    Ninja presets (default, windows-mingw)
.github/workflows/   CI matrix: Windows (MinGW) / Linux / macOS
```

### Adding a backend function

1. **C++** (`src/main.cpp`):
   `w.bind("myFunc", [](const std::string& req){ /* ... */ return json; });`
2. **Types** (`frontend/src/bridge.ts`): add `myFunc` to the `Window` interface and
   to the `backend` object.
3. **React**: `const r = await backend.myFunc(args);`

The C++ receives the JS arguments as a JSON array string and must return a JSON string,
which becomes the resolved value of the JS promise.

## The task runner

All day-to-day work goes through `tasks.py` (standard library only, cross-platform):

```bash
python tasks.py run-debug          # build + run in Debug
python tasks.py run-release        # build + run in Release
python tasks.py compile-release    # build Release + package a shareable zip
python tasks.py compile-release --release            # + create git tag vX.Y.Z
python tasks.py compile-release --release --publish  # + publish GitHub release (gh)
```

Each command runs the frontend build first, then the C++ build, using per-config
build directories (`build-debug/`, `build-release/`).

## How to compile

Manual build (what the task runner automates):

```bash
# 1. Frontend  ->  frontend/dist
cd frontend && npm install && npm run build && cd ..

# 2. C++ app  (copies frontend/dist -> <build>/ui)
cmake --preset windows-mingw     # Windows (MinGW)
cmake --preset default           # Linux / macOS
cmake --build build

# 3. Run
./build/app        # Linux/macOS
build\app.exe      # Windows
```

Or simply: `python tasks.py run-release`.

## How to debug

```bash
python tasks.py run-debug
```

This builds the frontend with **Vite development mode + source maps**
(`npm run build:debug`, unminified) and the C++ side with `CMAKE_BUILD_TYPE=Debug`
(symbols, no optimization), then launches the app.

- **C++ debugging:** the Debug binary lives in `build-debug/app`. Attach `gdb`
  (MSYS2 UCRT64 ships it): `gdb build-debug/app`.
- **Frontend debugging:** in the running app, open the webview dev tools
  (right-click → Inspect on platforms that expose it) — source maps map back to your
  original `.tsx`.
- **Fastest UI loop (hot reload):** run `cd frontend && npm run dev` (Vite at
  `http://localhost:5173`), temporarily point the app at it by changing
  `w.navigate(...)` in `src/main.cpp` to `w.navigate("http://localhost:5173");`,
  rebuild the C++ once, and edit React with live reload. Revert `navigate()` to
  `file://ui/index.html` before shipping.

## How to create artifacts to share

```bash
python tasks.py compile-release
```

Produces `artifacts/citepp-<version>-<os>.zip` containing the binary, the built `ui/`,
`README.md` and `LICENSE` — ready to hand to a user on the same OS.

To also cut a versioned release (recommended):

```bash
python tasks.py compile-release --release --publish
```

- `--release` creates an annotated git tag `vX.Y.Z` (version read from
  `frontend/package.json`).
- `--publish` uploads the zip as a GitHub Release using the `gh` CLI.

The version comes from `frontend/package.json` → bump it before cutting a release.

> Runtime note: on the end user's machine the Windows build needs the **WebView2
> runtime** (already present on Windows 10/11). Linux needs WebKitGTK libraries
> installed. Bundle/installer generation (`.msi` / `.AppImage` / `.dmg`) is a natural
> next step and can be wired into `compile-release`.

## CI

`.github/workflows/build.yml` builds the frontend, then the C++ app on
**Windows (MSYS2/MinGW)**, **Linux**, and **macOS**, and uploads the binaries as
build artifacts on every push and pull request.

## License

MIT — see [LICENSE](LICENSE).

[webview/webview]: https://github.com/webview/webview
[MSYS2]: https://www.msys2.org/
