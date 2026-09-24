# NSeqArpKeys

![NSeqArpKeys logo](assets/brand/logo.svg)

NSeqArpKeys is a pattern instrument for Windows, Linux, and macOS. It builds as
a standalone app and a VST3; macOS also has an Audio Unit. Each MIDI trigger
key has its own pattern, Forte pitch-class set, channel, octave, Gate, Fixed
Steps, optional subdivision, mode, and expression controls. Hold a key to play
its pattern, or turn on Latch to keep it
playing while you edit. The app produces MIDI notes and has an internal
preview sound.

## Download and install

Download the archive for your OS from the GitHub release. Each archive contains
the native standalone app, the VST3 bundle, the user manual, and SHA-256 hashes.
The macOS archive also contains an Audio Unit bundle.

| OS | Standalone | VST3 folder | Audio Unit folder |
| --- | --- | --- | --- |
| Windows x64 | `NSeqArpKeys.exe` | `C:\Program Files\Common Files\VST3` | — |
| Linux x64 | `NSeqArpKeys` | `~/.vst3` | — |
| macOS Intel / Apple Silicon | `NSeqArpKeys.app` | `~/Library/Audio/Plug-Ins/VST3` | `~/Library/Audio/Plug-Ins/Components` |

Copy the entire plug-in bundle into the indicated folder, then rescan plug-ins
in your DAW. The macOS build is universal; it is not signed or notarized.

Start with the **Single Note Pulse** factory preset and hold C4 (MIDI 60).
The pattern stops when the key is released. Turn on **Latch (keep playing)**
to hear it while editing. **Stop Key** and **Stop All** end playback.

The editable [HTML manual](docs/NSeqArpKeys-Manual.html) and
[printable PDF](output/pdf/NSeqArpKeys-Manual.pdf) cover all controls, Forte
sets, presets, and troubleshooting.

## Version 1.2.0 pattern editing

The **Global Steps/QN** control keeps its existing host parameter ID and default
value. **Steps/QN** on a key is 0 to inherit that value, or 1–16 to override it.
Two held keys may therefore have different rates. The meter numerator remains a
global grouping setting; the denominator control defines steps per quarter note.
Old projects without a per-key Steps/QN value continue to inherit the global
value.

Choose **Melodic** for Forte-mapped integer bitsets or **Rhythmic** for bits
0–15 mapped to the 16 editable drum MIDI notes (default channel 10 when
switching from channel 1). A velocity value scales with how hard the key is
struck. The velocity lane has values 0–127 (0 silences that event), and the
pitch lane offsets each step in semitones. Shorter lanes cycle within the
pattern, and both lanes restart at each pattern loop. **Rotate**
shifts the integer sequence in time; **Reverse** reverses its order. The lanes
do not reverse or rotate with it. **Copy**, **Paste**, and **Duplicate to next
key** work on complete independent assignments. **Assign range** copies the
selected key into the inclusive MIDI range; **Transpose by key** offsets each
copy by its distance from the source key. A range assignment is an independent
copy that can be edited afterward. Assign range is one Undo action. Undo/Redo
last for the current editor session and reset when a whole preset/project is loaded.

**Save pattern** and **Load pattern** use `.nseqpattern` files, separate from
whole performance `.nseqpreset` files. They open in the pattern-bank folder
under the application-data directory. Loading a pattern replaces the selected
key's assignment and restarts that key if active, preserving trigger velocity.
Other keys keep playing. Loading a pattern can be undone.

Changes to tempo, Global Steps/QN or a key's Steps/QN preserve musical phase;
other edits restart the affected key. Invalid integer input is outlined in red
and leaves the last valid value playing. Patterns and lanes accept up to 4096
space-separated integers.

The [release roadmap](docs/1.2.0-roadmap.md) defines the 1.2.0 feature set and
keeps the pattern-bank browser, linked ranges, advanced rhythm, modulation and
performance milestones for future releases. See the
[implementation status](docs/1.2.0-implementation-status.md) for validation.

## Note lengths

One step lasts `60 / (BPM × effective Steps/QN)` seconds. At 120 BPM and Steps/QN 4,
that is 0.125 seconds. For a sounding step, its span includes the following
zero steps up to the next sounding step, including across the loop boundary.

`note length in steps = Fixed Steps + Gate × sounding span`

Gate ranges from 0 to 2. Fixed Steps ranges from 0 to 16, in increments of
0.01. Both values add to the length; values above one step can make notes
overlap. For example, a span of 3 with Gate 0.5 and Fixed Steps 0.5 lasts
2 steps. Existing presets without Fixed Steps load with a value of 0.

## Presets

The preset browser has six factory examples plus searchable user presets.
Presets capture all 128 key assignments, including Gate and Fixed Steps. You
can save, update, duplicate, favourite, import, export, and delete user
presets. The standalone app and plug-ins share the OS application-data folder
`NSeqArpKeys/Presets`. DAW projects also retain their current plug-in state.
To share a preset separately, export a `.nseqpreset` file.

## Build from source

The CMake project uses JUCE 8.0.6. CMake 3.22 or newer and a C++17 compiler
are required. Set `JUCE_SOURCE_DIR` to use an existing JUCE checkout, or let
CMake fetch the pinned JUCE version.

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target NSeqArpKeys_Standalone
cmake --build build --config Release --target NSeqArpKeys_VST3
cmake --build build --config Release --target NSeqArpKeysTests NSeqArpKeysDomainTests NSeqArpKeysStateTests
ctest --test-dir build -C Release --output-on-failure
```

On macOS, also build `NSeqArpKeys_AU`. For a universal macOS build, configure
with `-DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"` and
`-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0`. Linux needs the JUCE development
packages listed in the [JUCE Linux dependencies guide](https://github.com/juce-framework/JUCE/blob/8.0.6/docs/Linux%20Dependencies.md).
The generated Visual Studio solution under `Builds/VisualStudio2022/` is also
available for Windows builds.

## Package and publish

After a Release build, run:

```text
python scripts/package-release.py --build-dir build --platform windows-x64
```

Use `linux-x64` or `macos-universal` for the matching native build. The script
checks the VST3 manifest version, includes the manual, writes SHA-256 hashes,
and verifies the ZIP. Archives are written to `release/`.

[The GitHub Actions workflow](.github/workflows/build-release.yml) builds and
runs this packaging script on Windows, Linux, and macOS. Every run uploads the
three ZIPs as workflow artifacts. Pushing a `v1.2.0` tag publishes them as a
GitHub release after all three builds succeed.

## Repository map

| Path | Purpose |
| --- | --- |
| `Source/` | Processor, editor, sequencing, Forte data, and preset code |
| `CMakeLists.txt` | Cross-platform JUCE build |
| `NSeqArpKeys.jucer` | JUCE project definition for the generated Windows solution |
| `assets/brand/` | Source logo, app icon, and platform icon assets |
| `docs/NSeqArpKeys-Manual.html` | Editable user manual |
| `output/pdf/NSeqArpKeys-Manual.pdf` | Printable user manual |
| `scripts/package-release.py` | Native distribution packaging and verification |
