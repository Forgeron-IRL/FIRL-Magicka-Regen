# FIRL Magicka Regen

SKSE plugin for Skyrim Special Edition / Anniversary Edition that turns magicka regeneration into a
linear formula, for the player and every NPC, configurable in game through an MCM:

```
Regen per second = max magicka × Percent / 100 + Flat
```

- Default: 3% of max + 5 per second.
- Same rate in and out of combat.
- Regeneration bonuses (enchantments, potions, perks) still apply on top.
- Nothing is written to the save: the plugin hooks each actor's `Update` (vfunc `0xAD` of
  `PlayerCharacter` and `Character`), lets the game run its own regeneration, then adds or removes the
  difference with the target formula.

## Requirements

- SKSE64 and [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- [MCM Helper](https://www.nexusmods.com/skyrimspecialedition/mods/53000) and
  [SkyUI](https://www.nexusmods.com/skyrimspecialedition/mods/12604) for the menu

Incompatible with other SKSE plugins that change magicka regeneration (Regeneration Overhaul,
Regen Adjuster SKSE).

## Repository layout

```
src/plugin.cpp          the whole plugin
assets/                 shipped as is: FIRLMagickaRegen.esl (MCM quest) and MCM/Config (MCM Helper menu)
extern/CommonLibSSE-NG  git submodule (alandtse/CommonLibSSE-NG v15.5.0)
triplets/               vcpkg overlay triplet pinning the MSVC v143 toolset
```

The ESL holds a single Start Game Enabled quest with `MCM_ConfigBase` attached directly
(`ModName = FIRLMagickaRegen`); no compiled Papyrus script is needed.

## Building

Requirements: Visual Studio with the **v143** toolset (C++ desktop workload), CMake, and
[vcpkg](https://github.com/microsoft/vcpkg) with `VCPKG_ROOT` set.

```
git clone --recursive https://github.com/Forgeron-IRL/FIRL-Magicka-Regen.git
cd FIRL-Magicka-Regen
cmake -B build -S . -A x64 -T v143 ^
  -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
  -DVCPKG_TARGET_TRIPLET=x64-windows-static-md ^
  -DVCPKG_OVERLAY_TRIPLETS=triplets
cmake --build build --config Release
```

The first configure installs the vcpkg dependencies and compiles CommonLibSSE-NG, which takes a while.
A post-build step packs `FIRLMagickaRegen-<version>.zip` (DLL + ESL + MCM files) at the repository root.
`build.bat` does the same with the author's local Visual Studio path.

VR is disabled (`ENABLE_SKYRIM_VR OFF`).

## License

GPL-3.0-or-later with the Modding Exception and GPL-3.0 Linking Exception, the same terms as
CommonLibSSE-NG. See [LICENSE](LICENSE) and [EXCEPTIONS.md](EXCEPTIONS.md).
