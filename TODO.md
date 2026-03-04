# TODO.md - StepMania 3.9 Modernization Roadmap

This document outlines opportunities to modernize the StepMania 3.9 codebase (originally from 2004-2005) to modern C++ standards and practices.

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Priority 1: Critical Security Fixes](#priority-1-critical-security-fixes)
3. [Priority 2: Build System Modernization](#priority-2-build-system-modernization)
4. [Priority 3: C++ Language Modernization](#priority-3-c-language-modernization)
5. [Priority 4: STL and Standard Library](#priority-4-stl-and-standard-library)
6. [Priority 5: Threading Modernization](#priority-5-threading-modernization)
7. [Priority 6: Architecture Improvements](#priority-6-architecture-improvements)
8. [Priority 7: Graphics Modernization](#priority-7-graphics-modernization)
9. [Appendix: File Reference](#appendix-file-reference)

---

## Executive Summary

**Codebase Statistics:**
- Total C++ files: ~411
- Lines of code: ~200,000+
- Original era: C++98/03

**Key Modernization Areas:**
| Area | Current State | Target | Effort |
|------|---------------|--------|--------|
| Build System | Autoconf 2.57 | CMake 3.10+ | HIGH |
| SDL | 1.2.6 (EOL 2012) | SDL 2.0+ | MEDIUM |
| C++ Standard | C++98/03 | C++17 | HIGH |
| Memory Management | Raw pointers | Smart pointers | HIGH |
| Threading | pthreads | std::thread | MEDIUM |
| OpenGL | Immediate mode | Shaders/VBOs | HIGH |

---

## Priority 1: Critical Security Fixes

### 1.1 Buffer Overflow Vulnerabilities (CRITICAL)

**Issue:** Unsafe string operations throughout codebase.

| File | Line | Issue | Fix |
|------|------|-------|-----|
| `archutils/Win32/GotoURL.cpp` | 19, 39, 57-58 | strcpy/strcat without bounds | Use snprintf/strncat |
| `RageThreads.cpp` | 159, 230, 232, 253 | strcpy into fixed buffers | Use std::string |
| `archutils/Win32/Crash.cpp` | 53, 55, 64, 153-154 | strcpy/strcat in crash handler | Use snprintf |
| `Model.cpp` | 223, 244 | strcpy with unknown buffer size | Use bounded copy |
| `arch/LowLevelWindow/LowLevelWindow_SDL.cpp` | 124-125 | strcpy/strcat for env var | Use setenv() |

**Action Items:**
- [ ] Replace all `strcpy` with `strncpy` or `std::string`
- [ ] Replace all `strcat` with `strncat` or `std::string::append`
- [ ] Replace all `sprintf` with `snprintf`
- [ ] Add bounds checking to all string operations

### 1.2 Format String Vulnerabilities

| File | Line | Issue |
|------|------|-------|
| `archutils/Win32/Crash.cpp` | 150, 978 | wsprintf with user data |
| `RageSurface_Load_PNG.cpp` | 94, 102 | sprintf without format args |

### 1.3 Memory Safety Issues

**Unsafe realloc patterns:**
```cpp
// src/RageSoundManager.cpp:421
mixbuf = (int32_t *) realloc( mixbuf, sizeof(int32_t) * realsize );
// Risk: Original pointer lost if realloc fails
```

**Action Items:**
- [ ] Check all realloc return values before assignment
- [ ] Add integer overflow checks before size calculations
- [ ] Replace manual memory management with smart pointers

### 1.4 Path Traversal Vulnerabilities

| File | Line | Issue |
|------|------|-------|
| `RageFileManager.cpp` | 118, 120 | chdir with "../" paths |
| `Song.cpp` | 1197-1207 | Incomplete "../" validation |
| `RageFileDriverDirectHelpers.cpp` | 277, 281, 290-291 | Multiple chdir operations |

**Action Items:**
- [ ] Implement path canonicalization
- [ ] Validate all file paths against traversal attacks
- [ ] Use absolute paths internally

---

## Priority 2: Build System Modernization

### 2.1 Migrate Autoconf to CMake

**Current:** Autoconf/Automake 2.57+
**Target:** CMake 3.10+

**Benefits:**
- Better IDE integration (Visual Studio, CLion, Xcode)
- Simplified dependency detection
- Faster configuration
- Better cross-platform support

**Action Items:**
- [ ] Create root `CMakeLists.txt`
- [ ] Convert `configure.ac` options to CMake variables
- [ ] Migrate `src/Makefile.am` to `src/CMakeLists.txt`
- [ ] Convert m4 macros to CMake find modules
- [ ] Add CPack support for packaging

### 2.2 Migrate SDL 1.2 to SDL 2.0

**Current:** SDL 1.2.6 (End-of-life 2012)
**Target:** SDL 2.0+

**Files to modify:**
- `arch/InputHandler/InputHandler_SDL.cpp/h`
- `arch/LowLevelWindow/LowLevelWindow_SDL.cpp/h`
- `arch/LoadingWindow/LoadingWindow_SDL.cpp/h`
- `SDL_utils.cpp/h`

**Key API changes:**
| SDL 1.2 | SDL 2.0 |
|---------|---------|
| `SDL_SetVideoMode()` | `SDL_CreateWindow()` + `SDL_CreateRenderer()` |
| `SDL_Flip()` | `SDL_RenderPresent()` |
| `SDL_EventState()` | Similar but different constants |
| Surface pixel access | Different pixel format handling |

**Action Items:**
- [ ] Create SDL2 abstraction layer
- [ ] Update event handling code
- [ ] Update window creation code
- [ ] Test all input handlers
- [ ] Update video resize handling

### 2.3 Replace Bundled Libraries with System Packages

| Library | Current | Recommendation | Effort |
|---------|---------|----------------|--------|
| libpng | Bundled in `src/libpng/` | System libpng-dev | LOW |
| zlib | Bundled in `src/zlib/` | System zlib1g-dev | LOW |
| PCRE | Bundled in `src/pcre/` | System or C++11 `<regex>` | MEDIUM |
| libresample | Custom in `src/libresample/` | libsamplerate or FFmpeg | MEDIUM |
| libmad | 0.15.1b in `src/mad-0.15.1b/` | FFmpeg avcodec | MEDIUM |
| Crypto++ | 5.1 in `src/crypto51/` | OpenSSL or Crypto++ 8.x | HIGH |

**Action Items:**
- [ ] Add CMake `find_package()` calls for each library
- [ ] Remove bundled source directories
- [ ] Update include paths
- [ ] Test with system libraries

---

## Priority 3: C++ Language Modernization

### 3.1 Replace NULL with nullptr

**Current:** 1,815 occurrences of `NULL` across 322 files

**Before:**
```cpp
Song* m_pCurSong = NULL;
if (ptr == NULL) { ... }
```

**After:**
```cpp
Song* m_pCurSong = nullptr;
if (ptr == nullptr) { ... }
```

**Action Items:**
- [ ] Global search/replace `= NULL` with `= nullptr`
- [ ] Replace `== NULL` with `== nullptr`
- [ ] Replace `!= NULL` with `!= nullptr`
- [ ] Update function default parameters

### 3.2 Replace Raw Pointers with Smart Pointers

**Current:** 906 occurrences of `new`/`delete` across 231 files

**Key files to refactor:**
| File | new/delete count | Priority |
|------|------------------|----------|
| `GameSoundManager.cpp` | 22 | HIGH |
| `RageDisplay_OGL.cpp` | 16 | HIGH |
| `ScreenManager.cpp` | 6 | HIGH |
| `GameState.cpp` | 3 | HIGH |

**Before:**
```cpp
Screen* pScreen = new ScreenGameplay();
// ... use pScreen ...
delete pScreen;
```

**After:**
```cpp
auto pScreen = std::make_unique<ScreenGameplay>();
// ... use pScreen ...
// Automatic cleanup
```

**Action Items:**
- [ ] Replace `SAFE_DELETE` macro usage with smart pointers
- [ ] Convert `vector<T*>` to `vector<unique_ptr<T>>`
- [ ] Add move constructors where appropriate
- [ ] Remove manual delete calls

### 3.3 Convert Unscoped Enums to enum class

**Current:** 40+ unscoped enum definitions

**Files with enums to convert:**
- `RageTypes.h`: GlowMode, BlendMode, CullMode, ZTestMode, PolygonMode
- `Actor.h`: TweenType, Effect, EffectClock
- `GameState.h`: HealthState
- `PrefsManager.h`: BackgroundModes, BannerCacheMode, Premium, Maybe, etc.
- `PlayerNumber.h`: PlayerNumber

**Before:**
```cpp
enum BlendMode { BLEND_NORMAL, BLEND_ADD, BLEND_NO_EFFECT };
```

**After:**
```cpp
enum class BlendMode { Normal, Add, NoEffect };
```

**Action Items:**
- [ ] Convert all enums to enum class
- [ ] Update all enum usage sites
- [ ] Remove redundant prefixes from enum values

### 3.4 Replace C-Style Casts

**Current:** 30+ C-style casts in critical paths

**Before:**
```cpp
const char *buf = (const char *)glGetString(GL_EXTENSIONS);
uint16_t value = *(uint16_t *)p;
```

**After:**
```cpp
const char *buf = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
uint16_t value = *reinterpret_cast<uint16_t*>(p);
```

**Action Items:**
- [ ] Replace with `static_cast<>` where type conversion is safe
- [ ] Replace with `reinterpret_cast<>` for pointer conversions
- [ ] Replace with `dynamic_cast<>` for polymorphic types
- [ ] Replace with `const_cast<>` for const removal (review necessity)

### 3.5 Replace typedef with using

**Current:** 15+ typedef declarations

**Before:**
```cpp
typedef uint8_t pixval;
typedef map<CString, CString> key;
```

**After:**
```cpp
using pixval = uint8_t;
using key = std::map<std::string, std::string>;
```

### 3.6 Replace Macros with constexpr

**Before:**
```cpp
#define PI (3.141592653589793f)
#define DRAW_ORDER_BEFORE_EVERYTHING -100
#define OPT_SAVE_PREFERENCES (1<<0)
```

**After:**
```cpp
constexpr float PI = 3.141592653589793f;
constexpr int DRAW_ORDER_BEFORE_EVERYTHING = -100;
constexpr unsigned int OPT_SAVE_PREFERENCES = 1u << 0;
```

---

## Priority 4: STL and Standard Library

### 4.1 Replace CString with std::string

**Current:** Custom `CStdStr` class (1,315 lines in `StdString.h`)
**Occurrences:** 1,163 across 266 files

**Phase 1: Add std::string compatibility**
```cpp
// Add to CStdStr
operator std::string() const { return std::string(c_str()); }
CStdStr(const std::string& s) : base_class(s.c_str()) {}
```

**Phase 2: Gradual replacement**
- Replace CString parameters with `const std::string&`
- Replace CString returns with `std::string`
- Remove MFC-style methods

**Action Items:**
- [ ] Create compatibility layer
- [ ] Replace in new code first
- [ ] Gradually migrate existing code
- [ ] Remove StdString.h when complete

### 4.2 Replace FOREACH Macros with Range-Based For

**Current:** 596 uses of FOREACH/FOREACH_CONST macros

**Before:**
```cpp
FOREACH(Song*, vSongs, s)
    (*s)->DoSomething();
```

**After:**
```cpp
for (auto* song : vSongs)
    song->DoSomething();
```

**Action Items:**
- [ ] Replace all FOREACH macros
- [ ] Remove Foreach.h when complete
- [ ] Use `auto` for iterator types

### 4.3 Use STL Algorithms

**Opportunities identified:**
- `std::find` / `std::find_if` for searches
- `std::transform` for conversions
- `std::copy` / `std::copy_if` for filtering
- `std::sort` with custom comparators
- `std::erase_if` (C++20) for container cleanup

**Before:**
```cpp
vector<Steps*>::iterator it = find(vSteps.begin(), vSteps.end(), pSteps);
if (it != vSteps.end())
    vSteps.erase(it);
```

**After:**
```cpp
std::erase(vSteps, pSteps);  // C++20
// Or for C++17:
auto it = std::find(vSteps.begin(), vSteps.end(), pSteps);
if (it != vSteps.end()) vSteps.erase(it);
```

### 4.4 Use emplace_back Instead of push_back

**Current:** 697 occurrences of push_back/insert

**Before:**
```cpp
m_vpSongs.push_back(new Song());
```

**After:**
```cpp
m_vpSongs.emplace_back(std::make_unique<Song>());
```

### 4.5 Add std::optional for Nullable Returns (C++17)

**Before:**
```cpp
int Find(char ch) const {
    size_t idx = find_first_of(ch);
    return idx == npos ? -1 : static_cast<int>(idx);
}
```

**After:**
```cpp
std::optional<size_t> Find(char ch) const {
    auto idx = find_first_of(ch);
    return idx != npos ? std::optional(idx) : std::nullopt;
}
```

---

## Priority 5: Threading Modernization

### 5.1 Replace pthreads with std::thread

**Current:** Custom `RageThread` class wrapping pthreads
**Files:** `RageThreads.cpp/h`, `arch/Threads/Threads_Pthreads.cpp`

**Before:**
```cpp
pthread_t thread;
pthread_create(&thread, NULL, ThreadFunc, data);
pthread_join(thread, NULL);
```

**After:**
```cpp
std::thread thread(ThreadFunc, data);
thread.join();
```

### 5.2 Replace Custom Mutex with std::mutex

**Current:** Custom `RageMutex` class
**Files:** `RageThreads.cpp/h`

**Before:**
```cpp
RageMutex mutex("MyMutex");
mutex.Lock();
// critical section
mutex.Unlock();
```

**After:**
```cpp
std::mutex mutex;
std::lock_guard<std::mutex> lock(mutex);
// critical section (automatic unlock)
```

### 5.3 Add Atomic Operations

**Current:** No use of `std::atomic`

**Opportunities:**
- Reference counts
- Boolean flags for thread control
- Statistics counters

**After:**
```cpp
std::atomic<bool> m_bRunning{true};
std::atomic<int> m_iFrameCount{0};
```

### 5.4 Replace Condition Variables

**Current:** Custom condition variable wrappers
**Target:** `std::condition_variable`

```cpp
std::condition_variable cv;
std::mutex mtx;

// Wait
std::unique_lock<std::mutex> lock(mtx);
cv.wait(lock, []{ return ready; });

// Notify
cv.notify_one();
```

---

## Priority 6: Architecture Improvements

### 6.1 Replace Global Singletons with Dependency Injection

**Current:** 19+ global singleton pointers

| Singleton | File | Usage |
|-----------|------|-------|
| SCREENMAN | ScreenManager.h | 85 |
| GAMESTATE | GameState.h | 297 |
| PREFSMAN | PrefsManager.h | 308 |
| TEXTUREMAN | RageTextureManager.h | 104 |
| SOUNDMAN | RageSoundManager.h | 160 |
| THEME | ThemeManager.h | 76 |
| LOG | RageLog.h | 43 |
| FILEMAN | RageFileManager.h | - |

**Phase 1: Create Service Container**
```cpp
class ServiceContainer {
    std::unique_ptr<GameState> m_gameState;
    std::unique_ptr<ScreenManager> m_screenManager;
    // ...
public:
    GameState& GetGameState() { return *m_gameState; }
    ScreenManager& GetScreenManager() { return *m_screenManager; }
};
```

**Phase 2: Constructor Injection**
```cpp
class ScreenGameplay {
public:
    ScreenGameplay(GameState& gs, SoundManager& sm)
        : m_gameState(gs), m_soundManager(sm) {}
private:
    GameState& m_gameState;
    SoundManager& m_soundManager;
};
```

### 6.2 Reduce Deep Inheritance Hierarchies

**Current:**
```
Actor (150+ lines, 64+ virtual functions)
  └── ActorFrame
        └── Screen (50+ virtual functions)
              └── ScreenWithMenuElements
                    └── ScreenGameplay
```

**Target: Composition over Inheritance**
```cpp
class Actor {
    TransformComponent m_transform;
    RenderComponent m_render;
    TweenComponent m_tween;
};
```

### 6.3 Modernize Error Handling

**Current:** Mixed (exceptions, asserts, error codes)

**Target: Unified approach with std::expected (C++23) or Result<T,E>**
```cpp
template<typename T>
class Result {
    std::variant<T, Error> m_result;
public:
    bool IsError() const;
    T Unwrap();
    Error GetError() const;
};
```

### 6.4 Add Unit Testing Framework

**Current:** Manual tests in `src/tests/` (7 files)

**Target:** Google Test or Catch2

**Action Items:**
- [ ] Add testing framework dependency
- [ ] Create test fixtures for common setups
- [ ] Add tests for critical components
- [ ] Set up CI/CD pipeline

---

## Priority 7: Graphics Modernization

### 7.1 Complete VBO Migration

**Current:** Partial VBO support, with immediate mode fallback
**File:** `RageDisplay_OGL.cpp`

**Action Items:**
- [ ] Enable VBO by default
- [ ] Remove vertex array fallback code
- [ ] Optimize buffer uploads

### 7.2 Implement Shader-Based Rendering

**Current:** Fixed-function pipeline only

**Phase 1: Basic GLSL Shaders**
```glsl
// vertex.glsl
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
uniform mat4 uModelViewProj;
out vec2 TexCoord;
void main() {
    gl_Position = uModelViewProj * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
```

**Phase 2: Effect Shaders**
- Glow effects
- Color manipulation
- Texture blending

### 7.3 Remove Deprecated GL Functions

**To Remove:**
- `glPushMatrix()` / `glPopMatrix()`
- `glColor4fv()`
- `glVertexPointer()` / `glTexCoordPointer()`
- `GL_EXT_paletted_texture`

### 7.4 Direct3D Modernization (Optional)

**Current:** Direct3D 8 (year 2000)
**Options:**
1. Keep for legacy support
2. Migrate to Direct3D 11
3. Add Vulkan support
4. Focus on OpenGL only

---

## Appendix: File Reference

### Key Files for Each Area

**Build System:**
- `configure.ac` - Main autoconf configuration
- `src/Makefile.am` - Source build rules
- `autoconf/m4/*.m4` - Detection macros

**Memory Management:**
- `src/RageUtil.h` - SAFE_DELETE macros
- `src/ScreenManager.h` - Screen pointer management
- `src/GameState.h` - Game state pointers

**String Handling:**
- `src/StdString.h` - CStdStr implementation
- `src/global.h` - CString typedef

**Threading:**
- `src/RageThreads.cpp/h` - Thread/mutex abstractions
- `src/arch/Threads/` - Platform implementations

**Graphics:**
- `src/RageDisplay_OGL.cpp` - OpenGL renderer
- `src/RageDisplay_D3D.cpp` - Direct3D 8 renderer
- `src/RageDisplay.h` - Abstract interface

**Security-Critical:**
- `src/archutils/Win32/Crash.cpp` - Crash handler
- `src/archutils/Win32/GotoURL.cpp` - URL handling
- `src/RageFileManager.cpp` - File system access

---

## Progress Tracking

### Completed
- [x] Compilation fixes for GCC 13/C++14
- [x] Git repository initialization
- [x] **Priority 1.1: Buffer Overflow Vulnerabilities** (2026-03-04)
  - [x] RageThreads.cpp: Fixed 6 strcpy/sprintf vulnerabilities (snprintf/strncpy)
  - [x] LowLevelWindow_SDL.cpp: Replaced strcpy/strcat/putenv with setenv()
  - [x] Model.cpp: Fixed 3 unbounded sscanf with buffer limits (%255[^\"])
- [x] **Priority 1.2: Format String Vulnerabilities** (2026-03-04)
  - [x] RageSurface_Load_PNG.cpp: Fixed sprintf without format specifiers
- [x] **Priority 1.3: Memory Safety Issues** (2026-03-04)
  - [x] RageSoundManager.cpp:421: Added realloc return check + overflow validation
  - [x] RageSurface.cpp:140,229: Added integer overflow checks before new[]
  - [x] RageSurfaceUtils_Palettize.cpp: Fixed memory leak (bv), replaced ASSERT with runtime checks
- [x] **Priority 1.4: Path Traversal Vulnerabilities** (2026-03-04)
  - [x] Song.cpp: Enhanced GetSongAssetPath() with absolute path rejection and escape detection
  - [x] NotesLoaderSM.cpp: Added IsValidAssetPath() validation for BANNER, BACKGROUND, MUSIC, LYRICSPATH, CDTITLE
- [x] CryptHelpers.h: Fixed byte typedef conflict (build fix)
- [x] **Priority 3.1: Replace NULL with nullptr** (2026-03-04)
  - [x] Converted ~250 occurrences in main source files (excluding third-party libraries)
  - [x] Files: Rage*, Screen*, Actor*, Song*, Notes*, Game*, Player*, Profile*, Course*
- [x] **Priority 3.2: Modernize FOREACH macros** (2026-03-04)
  - [x] Foreach.h: Updated macros to use auto and cbegin/cend
- [x] **Priority 3.5: Replace typedef with using** (2026-03-04)
  - [x] global.h: CCStringRef
  - [x] RageTypes.h: RectI, RectF
  - [x] RageUtil.h: longchar, istring
  - [x] Difficulty.h: CourseDifficulty
  - [x] GameInput.h: GameButton
- [x] **Priority 3.3: Enum modernization** (2026-03-04)
  - [x] RageTypes.h: 5 enums converted to enum class with compatibility aliases (GlowMode, BlendMode, CullMode, ZTestMode, PolygonMode)
  - [x] Added explicit underlying type (`: int`) to 36+ enums across:
    - Grade.h, Difficulty.h, PlayerNumber.h
    - GameConstantsAndTypes.h (14 enums)
    - GameInput.h, ScreenEvaluation.h, RageInputDevice.h
    - PrefsManager.h, ThemeManager.h, Course.h, Steps.h, Song.h

### In Progress
- [ ] Security audit and fixes (Windows-only files pending: GotoURL.cpp, Crash.cpp)
- [ ] Build system evaluation

### Not Started
- [ ] SDL 2.0 migration
- [ ] Smart pointer conversion
- [ ] Threading modernization
- [ ] Shader implementation

---

**Document Version:** 1.3
**Last Updated:** 2026-03-04
**Generated by:** Claude Code Analysis
