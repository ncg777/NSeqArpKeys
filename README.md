# NSeqArpKeys

![NSeqArpKeys logo](assets/brand/logo.svg)

NSeqArpKeys is a pattern instrument for Windows, Linux, and macOS. It builds as
a VST3 plug-in; macOS also has an Audio Unit. Each MIDI trigger
key has its own pattern, Forte pitch-class set, channel, octave, Gate, Fixed
Steps, optional subdivision, mode, and expression controls. Hold a key to play
its pattern, or turn on Latch to keep it
playing while you edit. The app produces MIDI notes and has an internal
preview sound.

**Create patterns with companion apps:**
[Sequence Operator](https://ncg777.github.io/sequence-operator/) shapes integer
sequences, [Rhythm Navigator](https://ncg777.github.io/rhythm-navigator/)
supplies rhythms and drum masks, and
[KComplexExplorer](https://ncg777.github.io/KComplexExplorer/) helps you choose
Forte pitch-class sets. Bring those ideas together in NSeqArpKeys.

## How integer patterns become notes

In **Melodic** mode, each space-separated integer in **Pattern** is one step.
Choose a **Forte Set** to supply the available pitch classes; the app repeats
those pitches in ascending order across octaves. It reads the magnitude of each
step as a binary mask, starting at the rightmost (least significant) bit. Bit 0
selects the first note at the **Octave** anchor, bit 1 the next, bit 2 the next,
and so on. Every selected note sounds together. A zero is a rest. A negative
value uses the same bits but walks downward from the anchor instead.

For example, choose `7-35.11`, the diatonic set with pitch classes
`[0 2 4 5 7 9 11]` (C D E F G A B), and set **Octave** to `4`. The anchor is
C (MIDI 48); the following notes are D (50) and E (52).

| Step | Binary magnitude | Selected notes |
| ---: | :---: | :--- |
| `1` | `001` | C |
| `2` | `010` | D |
| `4` | `100` | E |
| `3` | `011` | C + D |
| `5` | `101` | C + E |
| `7` | `111` | C + D + E |
| `0` | `000` | Rest |
| `-2` | `010` | B below the anchor |

Try `1 2 4 8 3 5 7 0` to hear a melody turn into overlapping chords. Keep
the numbers and change the Forte Set to hear the same bit pattern mapped onto
different pitches. The trigger key starts the assigned pattern; it does not
transpose these notes. **Transpose** changes the output pitch separately.
See the [manual's mapping guide](docs/NSeqArpKeys-Manual.html) and its complete
Forte pitch-class and interval-vector appendix.

## Make patterns with the companion apps

- **[Sequence Operator](https://ncg777.github.io/sequence-operator/)** creates
  and transforms integer sequences. Copy its result into a key's Melodic
  **Pattern** to turn the numbers into notes and chords.
- **[Rhythm Navigator](https://ncg777.github.io/rhythm-navigator/)** generates
  onset patterns. Place Sequence Operator values on those onsets and `0` on
  the rests. For drums, use its **Combined sequence → Copy** control and paste
  the decimal masks into a Rhythmic **Pattern**. Set **Velocity bits/lane** to
  `1`, use at most 16 lanes, put **Drum notes** in the displayed bit order, and
  match **Steps/QN** to its displayed denominator when that value is at most
  `16`.
- **[KComplexExplorer](https://ncg777.github.io/KComplexExplorer/)** lets you
  audition and compare pitch-class sets, inspect their Forte numbers and
  interval vectors, and generate harmonic matrices. Choose a set you like in
  NSeqArpKeys' **Find Set / Forte Set** controls; different trigger keys can
  carry different sets. If KComplexExplorer shows a padded suffix such as
  `.09`, look for the same transposition as `.9` in NSeqArpKeys.

Together, these tools let you choose the pitch collection, shape the integer
note stream, and design its timing before you play the result from a MIDI key.

## Download and install

Download the archive for your OS from the GitHub release. Each archive contains
the VST3 bundle, the user manual, and SHA-256 hashes.
The macOS archive also contains an Audio Unit bundle.

| OS | VST3 folder | Audio Unit folder |
| --- | --- | --- |
| Windows x64 | `C:\Program Files\Common Files\VST3` | — |
| Linux x64 | `~/.vst3` | — |
| macOS Intel / Apple Silicon | `~/Library/Audio/Plug-Ins/VST3` | `~/Library/Audio/Plug-Ins/Components` |

Copy the entire plug-in bundle into the indicated folder, then rescan plug-ins
in your DAW. The macOS build is universal; it is not signed or notarized.

Start with the **Single Note Pulse** factory preset and hold C4 (MIDI 60).
The pattern stops when the key is released. Turn on **Latch (keep playing)**
to hear it while editing. **Stop Key** and **Stop All** end playback.

Turn off **Preview sound** to silence the built-in tones while MIDI continues.
Route the plug-in's MIDI output to your instrument in the DAW and match its MIDI
channel to the assignment's **Channel**. The preview setting is saved with the setup.

The editable [HTML manual](docs/NSeqArpKeys-Manual.html) and
[printable PDF](output/pdf/NSeqArpKeys-Manual.pdf) cover all controls, Forte
sets, presets, and troubleshooting.

## Version 1.3.0 pattern editing

**Global Steps/QN** keeps its host parameter ID and default. The selected
key's **Steps/QN** is 0 to inherit the global value, or 1-16 to override it.
Multiple keys can play at different rates. Timing changes preserve phase.

Choose **Melodic** for Forte-mapped integers or **Rhythmic** for packed drum
lanes. Rhythmic mode accepts 1-16 MIDI pitches. **Velocity bits/lane** is one
number from 1-7, shared by every pitch. Lane 1 uses the lowest bits. The
default of one bit preserves older 16-lane masks: `5` triggers lanes 1 and 3.
Zero is a rest; nonzero lane levels map to velocity 1-127, then scale with
base and trigger velocity. With two lanes and two bits each, step `9` produces
levels 1/3 and 2/3. Positive decimal masks support all 16 seven-bit lanes;
legacy signed 32-bit masks still load. Set
**Channel** to match the receiving instrument; switching modes preserves it.

The editor shows the sequence length in parentheses after **Pattern**, a
preview of the first 16 steps, and active keys. It shows Forte and Octave
controls in Melodic mode and drum pitches and velocity bits in Rhythmic mode.
A dark theme uses pattern colours in the preview and pattern bank.

**Copy**, **Cut**, **Clear** and **Paste** support full assignments and selective
paste of sequence, timing, or expression. **Duplicate to next key** and
**Assign range** create independent copies. **Pattern Bank** searches names
and tags, filters favourites, and edits name, tags, colour and favourite
status. Assign a bank pattern as an independent copy or a shared link.
Linked keys follow edits to any member; **Make independent** breaks the link.
Right-click the keyboard to assign the selected bank pattern. Whole presets
embed shared pattern definitions, so exported setups reopen without the
local bank. Bank entries can be auditioned from the browser.

**Save pattern** and **Load pattern** use `.nseqpattern` files, separate from
whole `.nseqpreset` files. Loading one pattern affects only the selected key
and can be undone. Undo/Redo lasts for the editor session and resets on a
whole-preset or project restore.

Invalid integer input is outlined in red and leaves the last valid value
playing. Pattern input accepts at most 4096 signed integers and 45,056
characters. Names, tags, preset details, imported files and saved state are
also bounded before use.

The [1.3.0 release notes](docs/release-notes-1.3.0.md) summarize compatibility
and the [user manual](docs/NSeqArpKeys-Manual.html) explains the controls.

## Version 1.3.1 pattern bank transfer

The Pattern Bank can import and export all entries as one XML-based
`.nseqbank` file. Import merges into the local bank: matching pattern IDs are
updated and new IDs are added. Other local patterns are retained. An entry
without an ID receives one on import, so externally generated banks can omit
IDs. Export includes every bank entry, regardless of the current search or
favourites filter. See the [bank file format](docs/pattern-bank-format.md) and
[1.3.1 release notes](docs/release-notes-1.3.1.md).

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
presets. The plug-in formats share the OS application-data folder
`NSeqArpKeys/Presets`. DAW projects also retain their current plug-in state.
To share a preset separately, export a `.nseqpreset` file.

## Build from source

The CMake project uses JUCE 8.0.6. CMake 3.22 or newer and a C++17 compiler
are required. Set `JUCE_SOURCE_DIR` to use an existing JUCE checkout, or let
CMake fetch the pinned JUCE version.

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
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
three ZIPs as workflow artifacts. Pushing a `v1.3.1` tag publishes them as a
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
