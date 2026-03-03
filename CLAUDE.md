# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**StepMania 3.9** is a cross-platform rhythm game engine supporting Dance Dance Revolution, Pump It Up, EZ 2 Dancer, and Para Para Paradise game types. It features a highly modular architecture with a platform abstraction layer (Rage framework) providing graphics, sound, input, and file system services across Windows, Linux, macOS, and Xbox.

## Build System

**Build Tool**: GNU Autoconf/Automake (version 2.57+)

### Building on Modern Linux (Ubuntu 22.04+, GCC 13+)

The original 2005 codebase has been patched for modern compiler compatibility. Required steps:

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install build-essential autoconf automake libsdl1.2-dev \
    libpng-dev libjpeg-dev libvorbis-dev libogg-dev libmad0-dev \
    liblua5.1-dev libgl1-mesa-dev libglu1-mesa-dev libx11-dev

# Generate configure script (if not present)
autoreconf -fi

# Configure and build
./configure
make
```

### Building (General)

```bash
# Initial configuration (generate configure script and Makefile from templates)
./configure [OPTIONS]

# Build the game
make

# Build with specific options
./configure --with-debug           # Build with debug symbols
./configure --with-fast-compile    # Build with fast compilation
./configure --without-jpeg         # Disable JPEG support
./configure --without-network      # Disable networking features
./configure --enable-tests         # Build unit tests

# Clean build
make clean
make
```

### Modern Compilation Fixes Applied

The following compatibility fixes have been applied for GCC 13 / C++14:

| Issue | Fix Location |
|-------|--------------|
| Lua detection (pkg-config) | `autoconf/m4/lua.m4` |
| Missing X11 linkage | `autoconf/m4/opengl.m4` |
| C++14 standard flag | `src/Makefile.am` |
| Missing `<climits>` | `src/RageUtil.h` |
| Missing `<cstring>` | `src/global.h` |
| Extra class qualifications | `GameState.h`, `NetworkSyncServer.h`, `RageUtil_FileDB.h` |
| 64-bit pointer casts | `Threads_Pthreads.cpp`, `ScreenOptionsMasterPrefs.cpp` |
| Template two-phase lookup | `StdString.h`, `RageSurfaceUtils_Palettize.cpp` |
| libpng opaque structs | `RageSurface_Load_PNG.cpp` |
| `_syscall0` deprecated | `LinuxThreadHelpers.cpp` |
| `std::byte` conflict (C++17) | `crypto51/config.h`, `crypto51/misc.h` |
| glext.h conflicts | `RageDisplay_OGL.cpp` |
| unix/UNIX define | `LowLevelWindow_SDL.cpp` |

**Key Compilation Flags**:
- Release mode (default): `-O3` optimization
- Debug mode: `-g` flag for debugging
- Fast compile: `-O2 -fno-inline`
- Profiling: `--with-prof` adds `-pg` for gprof profiling

**Key Dependencies** (must be installed):
- SDL 1.2.6+
- libpng
- libjpeg (can be disabled with `--without-jpeg`)
- OpenGL or Direct3D drivers
- POSIX threads (pthreads)

**Optional Dependencies**:
- ALSA (Linux audio system)
- DirectSound (Windows audio)
- CoreAudio (macOS audio)
- GTK+ 2.0 (Linux loading window)
- Lua (scripting support)
- FFmpeg (video playback)
- Vorbis (Ogg audio support)

### Running

```bash
# After successful build:
./src/stepmania

# Run with specific theme or configuration:
./src/stepmania --theme ThemeName
./src/stepmania --arcade     # Arcade cabinet mode
```

### Testing

Tests can be built with `./configure --enable-tests` and are located in `src/tests/`.

## Codebase Architecture

### Layered Architecture (Top to Bottom)

1. **Game Screens/UI Layer** (138+ Screen* classes)
   - Screen-based state machine for menus, gameplay, evaluation
   - Message-passing between screens for loose coupling
   - Located in `src/Screen*.cpp/h`

2. **Game Logic & Manager Layer**
   - `GameState` - Central state repository (current song, player data, modifiers)
   - `ScreenManager` - Screen stack management
   - `SongManager` - Song/course database and loading
   - `ProfileManager` - Player profile persistence
   - `GameManager` - Game type definitions
   - Located in `src/*.cpp/h` (non-screen classes)

3. **Actor/Rendering Layer**
   - `Actor` base class for all visible objects
   - Hierarchical scene graph via `ActorFrame`
   - Sprite, BitmapText, Model for different render types
   - Tween system for smooth animation (multiple easing functions)
   - Located in `src/Actor*.cpp/h`

4. **Rage Framework Layer** (101+ Rage* classes)
   - Graphics: `RageDisplay`, `RageTexture`, `RageTextureManager`
   - Sound: `RageSoundManager`, `RageSoundDriver`, `RageSound`
   - Input: `RageInput`, `InputHandler`, `InputFilter`, `InputMapper`
   - Files: `RageFileManager`, `RageFile`, `RageFileDriver`
   - Math utilities, surface operations, audio resampling
   - Located in `src/Rage*.cpp/h`

5. **Platform Abstraction Layer**
   - `arch/` directory contains OS-specific implementations
   - Sound drivers: ALSA, DirectSound, OSS, CoreAudio, WaveOut, Software mixer
   - Input handlers: SDL-based device enumeration and polling
   - Graphics drivers: OpenGL and Direct3D implementations
   - Platform hooks for initialization and utilities
   - Located in `src/arch/*` and `src/archutils/*`

6. **External Libraries** (bundled and system)
   - Bundled: libresample, libmad, libpng, libjpeg, zlib, pcre, ffmpeg, crypto++
   - System: SDL, OpenGL, pthread, audio APIs, X11/Win32 windowing

### Key Subsystems

#### Graphics (Rendering)
- **Classes**: `RageDisplay`, `RageDisplay_OGL`, `RageDisplay_D3D`, `RageTexture`, `RageTextureManager`
- **Purpose**: Hardware-abstracted rendering with OpenGL and Direct3D support
- **Features**: Texture caching, vertex compilation, matrix operations, pixel format support

#### Sound (Audio)
- **Classes**: `RageSoundManager`, `RageSoundDriver`, `GameSoundManager`, `RageSound`
- **Drivers**: ALSA, DirectSound, OSS, CoreAudio, WaveOut, Software mixing
- **Readers**: WAV, MP3 (libmad), Ogg Vorbis support
- **Features**: Position tracking, soft-syncing, multi-threaded mixing, resampling

#### Input
- **Classes**: `RageInput`, `InputHandler`, `InputFilter`, `InputMapper`, `InputQueue`
- **Devices**: Keyboard, joysticks, dance pads, arcade controllers
- **Pipeline**: Raw input → device mapping → filtering → game interpretation → queue
- **Features**: Arcade cabinet coin detection, menu/gameplay button mapping

#### Gameplay
- **Note System**: `NoteData`, `NoteField`, `NoteDisplay`, actor-based rendering
- **Scoring**: `ScoreKeeper*` (different modes: MAX2, Rave), `HighScore`, `StageStats`
- **Player Control**: `Player` class manages single-player gameplay
- **Visual Feedback**: Receptors, ghost arrows, judgments, combos, life meters

#### File System
- **Drivers**: Direct filesystem, ZIP archives, in-memory filesystem
- **Song Formats**: .SM (native), .DWI, .BMS, .KSF
- **Image Formats**: PNG, JPEG, BMP, GIF, XPM
- **Audio Formats**: WAV, MP3, OGG, OGG Vorbis

#### Theme & UI
- **ThemeManager** - Resources and customization
- **MetricSystem** - Text and numeric parameters driving UI behavior
- **NoteSkinManager** - Visual style customization
- **FontManager** - Character and font management

### Important Directories

| Path | Purpose |
|------|---------|
| `src/` | Main source code (292 .cpp/.h files) |
| `src/arch/` | Platform abstraction: Sound, Input, Graphics, Dialog, MemoryCard, Lights, MovieTexture, LowLevelWindow, ArchHooks, Threads |
| `src/archutils/` | Platform-specific code: Win32/, Unix/, Darwin/, Xbox/ |
| `src/crypto/`, `crypto51/` | Cryptography libraries (MD5, SHA, RSA, crypto++) |
| `src/pcre/` | Regular expression matching |
| `src/libpng/`, `libjpeg/`, `zlib/` | Image/compression libraries |
| `src/libresample/`, `mad-0.15.1b/` | Audio libraries |
| `src/ffmpeg/` | Video codec support |
| `src/tests/` | Unit test code |
| `src/verify_signature/` | Arcade signature verification |
| `autoconf/` | Build system macros and utilities |

## Common Development Tasks

### Adding a New Screen

1. Create `ScreenMyScreen.h` and `ScreenMyScreen.cpp` extending `Screen`
2. Implement key methods: `Init()`, `Update()`, `Input()`, `HandleScreenMessage()`
3. Define layout and actors in `Init()`
4. Register in `Screen::Create()` static factory method
5. Add screen transitions from other screens using message passing

**Example Pattern**:
```cpp
class ScreenMyScreen : public Screen {
    virtual void Init();
    virtual void Update(float fDeltaTime);
    virtual void Input( const InputEventArray& input );
    virtual void HandleScreenMessage( const ScreenMessage &SM );
};
```

### Adding a New Game Type

1. Create game definition extending `Game` class
2. Define button mappings (style/controller layout)
3. Define scoring rules
4. Register in `GameManager::GetGames()`
5. Create `Style` variants (single/double/etc.)

### Adding a Gameplay Modifier

1. Add to `PlayerOptions` enum/flags
2. Implement effect in `NoteDataUtil::TransformNotes()` or gameplay actor
3. Add UI in options screens (`ScreenPlayerOptions`, etc.)
4. Save/load via profile system

### Porting to a New Platform

1. Add platform detection in `configure.ac`
2. Implement `src/arch/` drivers (Sound, Input, Graphics)
3. Add `src/archutils/` platform utilities
4. Implement `LowLevelWindow`, audio driver, input handler
5. Test with `./configure --host=your-platform`

### Debugging

- **Debug Build**: `./configure --with-debug && make clean && make`
- **Logging**: `RageLog` class provides logging to `stepmania.log`
- **Test Screens**: `ScreenTest*` classes available for unit testing (enable with config)
- **Debug Assertions**: `ASSERT()` macro for runtime checks

## Code Organization Patterns

### Singleton Pattern
All major subsystems are global singleton pointers (accessed via `extern`):
- `SCREENMAN` (ScreenManager)
- `INPUTMAN` (RageInput)
- `DISPLAY` (RageDisplay)
- `SOUNDMAN` (RageSoundManager)
- `SONGMAN` (SongManager)
- `GAMESTATE` (GameState)
- `PROFILE` (Profile)
- `FILEMAN` (RageFileManager)
- `THEME` (ThemeManager)
- `NOTESKINMAN` (NoteSkinManager)

### Message-Based Communication
Screens communicate via asynchronous message passing:
```cpp
SCREENMAN->PostMessageToTopScreen( ScreenMessage(SM_GoToNextScreen) );
SCREENMAN->PostMessageToTopScreen( ScreenMessage(SM_GoToNextScreen), DELAY_SECONDS );
```

### Tween-Based Animation
All actor animation uses unified tween system:
```cpp
actor->BeginTweening( duration, easing_type );
actor->SetTweenTarget( final_state );
```

### Data-Driven Configuration
Themes and preferences stored as text metrics:
```cpp
float val = THEME->GetMetricF( "MetricGroup", "MetricName" );
CString str = THEME->GetMetric( "MetricGroup", "MetricName" );
```

## Important Implementation Details

### Actor Coordinate System
- Origin (0,0) is typically center of screen
- X increases rightward, Y increases downward
- Z increases toward camera (out of screen)
- All transforms are applied relative to actor's anchor point

### Note Timing
- Timing data tied to `TimingData` per song
- Offsets for visual/audio sync adjusted via theme metrics
- `GameState` tracks playback position and beat information

### Screen Transitions
- Screens managed as stack by `ScreenManager`
- New screen pushed → old screen still running
- Message passing allows multiple screens to coordinate during transition
- Deferred messages enable timing control of transitions

### Cryptography
Arcade builds support signature verification for content protection:
- `CryptManager` handles RSA verification
- `verify_signature/` directory contains verification tools
- Signatures stored in `.smzip` packages

## Key Files to Review

- **Entry Point**: `src/StepMania.cpp`, `src/StepMania.h`
- **Global Setup**: `src/global.cpp`, `src/global.h`
- **Game Core**: `src/GameState.h`, `src/ScreenManager.h`
- **Screen Base**: `src/Screen.h`, `src/Screen.cpp`
- **Actor Base**: `src/Actor.h`, `src/Actor.cpp`
- **Rage Framework**: `src/Rage*.h` files (80+ core graphics/sound/input classes)
- **Build Config**: `configure.ac`, `src/Makefile.am`

## Notes for Contributors

- **Code Style**: Class names capitalized, variables camelCase, member variables prefixed with `m_`
- **Headers**: All .cpp files have corresponding .h header
- **Dependencies**: Minimal cross-dependencies enforced via module organization
- **Performance**: Compilation inlining limited to 300 to manage compile time
- **Platform Code**: Use `#ifdef LINUX`, `#ifdef WIN32`, etc., or better: use abstraction layer in `arch/`

## References

- **Documentation**: `README-FIRST.html` (comprehensive user and developer documentation)
- **Version History**: `NEWS` file
- **License**: `COPYING.txt` (all rights reserved, StepMania team 2001-2004)
