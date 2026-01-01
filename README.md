# Piano DAW

피아노 전용 미니 DAW - 피아노롤 + 스마트 퀀타이즈 + VSTi 호스팅 + LLM 코파일럿

## Features

- 🎹 Piano Roll Editor (Single track)
- 🎵 Smart Quantize with LLM copilot
- 🔌 VSTi Plugin Hosting
- 💾 Project Save/Load

## Build Requirements

- CMake 3.15+
- C++17 compiler
- JUCE 7.0.12 (auto-downloaded via CMake)

## Build Instructions

### Windows (Visual Studio)
```powershell
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

### macOS/Linux
```bash
cmake -B build
cmake --build build --config Release
```

## Run

```powershell
./build/PianoDAW_artefacts/Release/PianoDAW.exe  # Windows
./build/PianoDAW_artefacts/Release/PianoDAW      # macOS/Linux
```

## Development Roadmap

See [implementation_plan.md](docs/implementation_plan.md) for detailed roadmap.

## License

TBD (VST3 license decision required before distribution)
