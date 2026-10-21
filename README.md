# Monster Hunter Stories → PS Vita (so-loader port, work in progress)

This is a starting point for a Vita port of the **Android** release of Monster Hunter Stories (`jp.co.capcom.mhsspen`, v1.0.5). It runs the game's original 32-bit ARM library (`libMHR.so`) on the Vita through [soloader-boilerplate](https://github.com/v-atamanenko/soloader-boilerplate), with vitaGL for graphics.

> **Status: scaffold, never built or run.** The code is written against the boilerplate's APIs but has not been compiled or tested on hardware. Expect a debugging phase: crash logs, missing JNI methods, graphics fixes. You need your own legally obtained APK and OBB files. Nothing from the game is included here.

## What we know about the game

| | |
|---|---|
| Engine | Capcom MT Framework Mobile (`MTFP*` Java layer) |
| Code | `lib/armeabi-v7a/libMHR.so`, 23 MB, **not stripped** (~106k C++ symbols) |
| Graphics | OpenGL ES 2.0 only; GLSL shaders compiled at runtime; PVRTC/ETC1 aware |
| Audio | OpenSL ES (+ bundled libvorbis) |
| Data | `main.5` + `patch.5` OBBs (custom `.OBB` container, ~3.1 GB) + `assets/nativeAndroid` |
| Entry | Plain C exports `notifyOnCreate`, `notifyOnSurfaceChanged`, `renderFrame`, `notifyEvent`, … |
| Input | `notifyEvent(MTFPEvent)` with `ActivityGamePad` / `ActivityGesture` / `ActivityOnBackKey` |

The game has **built-in controller support**: an `sPad` key-mapping system with Shield, Nyko and generic Android profiles, plus a key-assign UI. The port uses it directly by sending the same gamepad events Android would.

## Layout

```
setup.sh                     clones the boilerplate and applies overlay/
overlay/source/main.c        lifecycle loop and input → MTFPEvent translation
overlay/source/java.c        fake Java layer (FalsoJNI): paths, device info, stubs
overlay/source/patch.c       kuser atomics patch + game hooks (by symbol name)
overlay/source/mhs/          MTFPEvent emulation, extra dynlib imports
tools/prepare_data.ps1       builds the ux0:data/mhs/ folder from your APK + OBBs
```

## Building with GitHub Actions (no local setup)

Push this folder to a GitHub repository. Every push runs [`.github/workflows/build.yml`](.github/workflows/build.yml) inside the official `vitasdk/vitasdk-softfp` Docker image and produces two downloads under **Actions → latest run → Artifacts**:

- `mhs_vita-Debug`: VPK with loader logging. Use this while getting the game to boot.
- `mhs_vita-Release`: optimized VPK.

Each also includes the `.elf`, for decoding crash dumps, and `BUILD_INFO.txt`, which records the exact commits used. Pushing a tag such as `v0.1.0` also creates a GitHub Release with the VPKs attached. To build against a specific boilerplate commit, use **Run workflow** and set `boilerplate_ref`.

Only the loader source goes in the repository. `.gitignore` blocks `*.so`, `*.obb` and `*.apk`, so game files can't be committed by accident.

## Building locally

You need Linux, macOS or WSL with [VitaSDK](https://vitasdk.org) installed (softfp toolchain, `$VITASDK` set), plus `git` and `cmake`.

```bash
./setup.sh
```

```bash
cmake -S build-tree -B build-tree/build -DCMAKE_BUILD_TYPE=Debug -DDUMP_COMPILED_SHADERS=ON
```

```bash
cmake --build build-tree/build -j
```

This produces `build-tree/build/mhs_vita.vpk`. Use `Debug` while bringing the port up, since it enables loader logging, and `Release` once it runs.

## Installing on the Vita

1. Install the **kubridge** and **fd_fix** plugins (ux0:tai/config.txt, under `*KERNEL`), and **libshacccg** (the runtime shader compiler; extract it with the standard SceShaccCg extractor). Reboot.
2. On Windows, stage the data:
   ```powershell
   powershell -ExecutionPolicy Bypass -File tools\prepare_data.ps1 -Apk "E:\downloads\MONSTER-HUNTER-STORIES-v1.0.5-full-apkvision.apk" -ObbDir "E:\downloads\MONSTER-HUNTER-STORIES-v1-0-5-cache\jp.co.capcom.mhsspen" -Out "E:\downloads\mhs_vita_data"
   ```
3. Copy the resulting `mhs` folder to `ux0:data/`, so you have `ux0:data/mhs/libMHR.so`, `ux0:data/mhs/assets/…` and `ux0:data/mhs/Android/obb/jp.co.capcom.mhsspen/*.obb`.
4. Install `mhs_vita.vpk` with VitaShell.

## Controls

The Vita reports itself to the game as an NVIDIA Shield controller, so the game uses its Shield button profile. You can also remap buttons in the game's own key-assign menu.

| Vita | Android key | Notes |
|---|---|---|
| Left stick | AXIS_X / AXIS_Y | movement |
| Right stick | AXIS_Z / AXIS_RZ | camera |
| D-pad | DPAD_* | menus |
| ✕ / ○ / □ / △ | BUTTON_A / B / X / Y | |
| L / R | BUTTON_L1 / R1 | |
| Start / Select | BUTTON_START / SELECT | |
| Touchscreen | gestures (tap, drag, long press) | the original touch UI still works |

The button table lives in the boilerplate's `source/reimpl/controls.c`. L2/R2/L3/R3 aren't mapped yet; rear-touch zones would be the natural place for them.

## Performance choices so far

- CPU 444 MHz, GPU 222 MHz, bus 222 MHz (maximum standard clocks).
- The game is told it's running on a 3-core device with 256 MB available, to steer MT Framework toward its lighter quality settings (`getAvailMemory`, `availableProcessors` in `java.c`).
- Shaders are compiled once and cached to disk (`DUMP_COMPILED_SHADERS=ON`), so later launches skip the compile stutter.
- Online, Google Play Games, Bluetooth and QR features report "unavailable", so no network or service code runs.

## Bring-up checklist (in order)

1. **Boot to first log line.** Fix any unresolved imports: the loader logs them, so add them to `overlay/source/mhs/dynlib_extra.inc`.
2. **Verify the entry-point ABI.** `mhs_native.h` assumes the boot library passes `(env, clazz, …)` through to `libMHR.so` unchanged. Confirm by opening `Java_jp_co_capcom_mhsspen_MTFPJNI_notifyOnCreate` in `libAndroidSkeletonBoot.so` in Ghidra.
3. **Missing JNI methods.** Read the FalsoJNI log for `GetMethodID` misses and add them to `java.c`.
4. **OBB found and opened.** Check the path printed by the game (`External Storage root = …`).
5. **First frame.** If shaders fail to compile, vitaGL's log names the shader; patch its GLSL in a `glShaderSource` hook.
6. **Input.** Check that buttons move the cursor. If the right stick does nothing, try AXIS_RX/RY in `mtfp_event.c`.
7. **Movies.** Currently stubbed (zero duration). Either play them with SceAvPlayer or hook the movie task to finish immediately.
8. **Memory.** If it runs out of memory, lower `_newlib_heap_size_user` or vitaGL's RAM share, and look at the engine's resource-cache sizes.
9. **Performance.** Profile with Razor; candidates are render resolution, shadows and the particle budget, all reachable by symbol.

## Shader compiler "fatal internal error" (builds 19–28; fix in build 29)

SceShaccCg (the runtime shader compiler) compiles and links a trivial test shader at startup, then fails on the same shader once the game has initialized. What the logs and the decompiled `libMHR.so` (Ghidra) establish:

| Build | Result |
|---|---|
| 19–23 | Game's first shader link fails; trivial test shaders also fail at that point |
| 24 | Fails on a fresh native worker thread too, so it's **process-wide state**, not stack, TLS or FPU flags |
| 25–26 | OK at startup, broken after `notifyOnCreate` |
| 27 | OK after `setJavaVM` (S1), broken after `native::initialize` (S2); free user memory 24,576 → 22,528 KB |

`native::initialize` calls 12 sub-inits. From the decompiled code, `debug`, `initRootPath`, `clipboard` and `console` are empty or only set variables, `httpclient` only registers a callback, and `performance` only calls `clock_getres`. The only process-wide effects are FalsoJNI calls (`android`, `filesystem`), one `malloc` plus a `/proc/meminfo` read (`memory`), and **two game threads** (File access manager, Multimedia) started through `pthread_create_soloader`, each with a 512 KB stack. `libMHR.so` never touches the TLS register directly (scanned in both Thumb and ARM mode).

Builds up to 27 also called `shark_set_allocators(malloc, free)` after `gl_init()`. That had no effect: vitaShaRK only hands the allocator to the module inside `shark_init()`, which vitaGL had already run with `vglMalloc`/`vglFree`.

**Build 28 result:** threads alone (T0/T1) were fine, and the largest free block stayed around 19 MB throughout, so it isn't fragmentation. Every sub-init up to `multithread` passed; the compiler broke right after **`filesystem::initialize` (S2.5)**, and the process crashed inside that link.

**Root cause:** `FileAccessManager`'s constructor builds **512 `native::multithread::Event`s**, each running `pthread_mutex_init` and `pthread_cond_init`. The boilerplate's pthread emulation (`reimpl/pthr.c`) records every initialized object in `initializedObjects[PTHR_MAX_OBJECTS]`, a table of **1024** entries. Once it's full, `rememberObject` fails silently and nothing checks the result. From then on, every lock of an untracked mutex looks uninitialized, so the boilerplate creates a new real mutex, backed by kernel objects, and leaks the old one. The File access manager thread enters its critical section every 10 ms, so kernel objects run out within moments, and those mutexes no longer exclude anything.

**Build 29 fix:**

- **Lazy Events:** `patch.c` hooks `Event::Event` and `Event::~Event`. The constructor leaves the mutex and cond at the bionic static initializer (0), and the boilerplate creates them on first use, so only Events the game actually uses cost anything. The destructor skips Events that were never used.
- **Bigger table:** `setup.sh` raises `PTHR_MAX_OBJECTS` to 4096.
- **Logging:** every self-test now logs `pthread objects N/4096`. If N climbs toward the limit, something else is creating objects in bulk.
- **Removed:** the 6 MB contiguous reserve from build 28, since fragmentation was ruled out.

## Credits

## Credits

- so-loader concept: TheFloW. FalsoJNI and soloader-boilerplate: Volodymyr Atamanenko. vitaGL: Rinnegatamante.
- Monster Hunter Stories © CAPCOM. This project contains no game code or assets.
