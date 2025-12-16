# MelodyForge Pro

JUCE-based MIDI chord + melody generator instrument plugin (VST3/AU) and standalone app.

## Requirements

- CMake 3.22+
- C++17 compiler
- JUCE as a submodule at `External/JUCE` (JUCE 7.0.5+)
- Python 3 (for generating placeholder embedded assets)

## Build (Release)

```bash
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release
```

## Outputs

- JUCE build artefacts: `build/MelodyForgePro_artefacts/`
- CI/package outputs: `dist/windows/` and `dist/macos/`

## Notes

- Embedded assets are procedurally generated placeholders (500 MIDI files + 200 JSON synth presets) and compiled into `BinaryData`.
- Assets are gzip-compressed before embedding; `AssetLibrary` transparently decompresses at runtime.
- Default macOS signing is ad-hoc (`codesign -s -`). Real signing/notarization is optional via CI secrets.
