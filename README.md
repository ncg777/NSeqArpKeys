# NSeqArpKeys

NSeqArpKeys is a Windows x64 pattern instrument available as a standalone app
and a VST3. Each MIDI trigger key can have its own step pattern, Forte
pitch-class set, channel, octave, and gate. Hold a key to play its pattern, or
turn on Latch to keep it playing while you edit. The app produces MIDI notes and
has an internal preview sound.

## Download and install

The ready-to-share archive is in [`release/`](release/). It contains:

```text
NSeqArpKeys.exe
NSeqArpKeys.vst3/
  Contents/
    Resources/moduleinfo.json
    x86_64-win/NSeqArpKeys.vst3
NSeqArpKeys-Manual.pdf
START-HERE.txt
SHA256SUMS.txt
```

- **Standalone:** Extract the ZIP and run `NSeqArpKeys.exe`. Choose an audio
  device from **Options** if necessary.
- **VST3:** Copy the entire `NSeqArpKeys.vst3` folder to
  `C:\Program Files\Common Files\VST3`, rescan plug-ins in your DAW, and insert
  NSeqArpKeys as an instrument. Keep the bundle's internal folders intact.

Start with the **Single Note Pulse** factory preset and hold C4 (MIDI 60). The
pattern stops when the key is released. Turn on **Latch (keep playing)** to
hear it while editing, and use **Stop Key** or **Stop All** to end playback.

For all controls, pattern notation, gate timing, Forte sets, presets, and
troubleshooting, read the [user manual](output/pdf/NSeqArpKeys-Manual.pdf).
The editable [HTML source](docs/NSeqArpKeys-Manual.html) is also in the repo.

## Presets

The preset browser has six factory examples plus searchable user presets.
Presets capture the complete setup, including all 128 key assignments. You can
save, update, duplicate, favourite, import, export, and delete user presets.
They are shared by the standalone app and VST3 at
`%APPDATA%\NSeqArpKeys\Presets`. DAW projects also retain their current plug-in
state. To share a preset separately, export a `.nseqpreset` file.

## Build from source

Requirements:

- Visual Studio 2022 with the v143 C++ toolset and Windows SDK
- JUCE 8.0.6 installed at `C:\JUCE` (the generated projects use
  `C:\JUCE\modules`)

Open `Builds/VisualStudio2022/NSeqArpKeys.sln` and build **Release | x64**,
or run:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe' `
    Builds/VisualStudio2022/NSeqArpKeys.sln `
    /p:Configuration=Release /p:Platform=x64 /m
```

Build outputs land under `Builds/VisualStudio2022/x64/Release/`. The GitHub
Actions workflow in `.github/workflows/build-windows.yml` builds and stages
the Windows binaries under `dist/windows-x64/`. If building locally, update
those staged binaries before packaging.

## Make a distribution ZIP

The package script reads the staged binaries from `dist/windows-x64/` and the
manual PDF from `output/pdf/`. It checks that the files and VST3 manifest are
present, uses the manifest version in the ZIP name, adds installation notes and
SHA-256 hashes, and verifies the completed archive.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\package-windows.ps1
```

The result is `release/NSeqArpKeys-<version>-windows-x64.zip`. Pass
`-OutputDirectory <path>` to write it elsewhere. The script packages existing
staged binaries; it does not compile the plug-in.

## Repository map

| Path | Purpose |
| --- | --- |
| `Source/` | Processor, editor, sequencing, Forte data, and preset code |
| `NSeqArpKeys.jucer` | JUCE project definition |
| `Builds/VisualStudio2022/` | Generated Visual Studio solution |
| `dist/windows-x64/` | Staged standalone and VST3 binaries |
| `docs/NSeqArpKeys-Manual.html` | Editable user manual |
| `output/pdf/NSeqArpKeys-Manual.pdf` | Printable user manual |
| `scripts/package-windows.ps1` | Distribution packaging and verification |
