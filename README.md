# NSeqArpKeys

A JUCE audio plug-in built with Visual Studio 2022 targeting Windows x64.

## Windows x64 Binaries

Pre-built Windows 64-bit (x64) Release binaries are committed under:

```
dist/
└── windows-x64/
    ├── VST3/
    │   └── NSeqArpKeys.vst3/          ← VST3 plugin bundle
    │       └── Contents/
    │           ├── Resources/
    │           │   └── moduleinfo.json
    │           └── x86_64-win/
    │               └── NSeqArpKeys.vst3   ← the DLL inside the bundle
    └── Standalone/
        └── NSeqArpKeys.exe            ← standalone application
```

These binaries are produced automatically by the **Build Windows x64** GitHub
Actions workflow (`.github/workflows/build-windows.yml`) and committed back to
the repository on every push to `master` or a `copilot/**` branch.

### Installation

- **VST3**: Copy `dist/windows-x64/VST3/NSeqArpKeys.vst3` into your system
  VST3 folder (typically `C:\Program Files\Common Files\VST3`). The plugin is
  exported as an instrument/synth and also emits its generated MIDI pattern
  notes.
- **Standalone**: Run `dist/windows-x64/Standalone/NSeqArpKeys.exe` directly.
  The standalone build includes an internal preview synth so assigned patterns
  can be auditioned without a separate host instrument.

## Rebuilding from Source

### Requirements

| Tool | Version |
|------|---------|
| Visual Studio | 2022 (Build Tools v143, Windows SDK 10.0) |
| JUCE | 8.0.6 |

JUCE must be cloned/installed to **`C:\JUCE`** so that the path
`C:\JUCE\modules` is valid. The vcxproj files reference this path directly.

```cmd
git clone --depth=1 --branch=8.0.6 https://github.com/juce-framework/JUCE.git C:\JUCE
```

### Build steps

Open `Builds/VisualStudio2022/NSeqArpKeys.sln` in Visual Studio 2022 and
build the **Release | x64** configuration, **or** run MSBuild from the
command line:

```cmd
msbuild Builds\VisualStudio2022\NSeqArpKeys.sln ^
        /p:Configuration=Release /p:Platform=x64 /m
```

Output artifacts land in `Builds/VisualStudio2022/x64/Release/`.
