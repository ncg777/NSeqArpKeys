# Pattern Bank file format

A `.nseqbank` file is UTF-8 XML. Its root is `NSeqPatternBank` with a `version`
attribute of `1`. Each direct child is an `NSeqPattern` with `version="1"` and
one `Assignment` child. These `NSeqPattern` elements use the same structure as
individual `.nseqpattern` files.

The optional `id` attribute on `NSeqPattern` is a stable identifier of at most
80 characters. IDs must be unique within a bank file. Importing an ID that is
already in the local bank updates that entry; a new ID adds an entry. When `id`
is omitted, the importer generates one and adds a new entry. Export always
includes IDs. IDs are case sensitive; any spaces in an ID are significant and
are preserved. Bank import does not delete local entries absent from the file.

`Assignment` must have a `sequence` attribute containing up to 4096
space-separated signed decimal steps (or positive decimal rhythmic masks up
to 112 bits). It may be empty. Other supported attributes are `mode`
(`melodic` or `rhythmic`), `name`, `tags`, `colour`, `favourite`, `forte`,
`channel`, `octave`, `gate`, `fixedLengthSteps`, `subdivision`, `transpose`,
`velocity`, `rotation`, `reverse`, `rootKey`, `drumNotes`, and
`drumVelocityBits`. The sequence text is limited to 45,056 bytes in UTF-8.
The optional `drumNotes` attribute contains 1–16 space-separated MIDI note
numbers from 0–127; an explicitly empty value is invalid. Omitted optional
attributes use the defaults below. The `linkId` attribute, if
present, is ignored during bank import because project links are kept in the
project state.

| Attribute | Accepted values | Default when omitted |
| --- | --- | --- |
| `mode` | `melodic` or `rhythmic` | `melodic` |
| `channel` | Integer 1–16 | 1 |
| `octave` | Integer 0–10 | 4 |
| `subdivision` | Integer 0–16; 0 inherits the global rate | 0 |
| `transpose` | Integer -127–127; Forte-set degrees in melodic mode, semitones in rhythmic mode | 0 |
| `velocity` | Integer 1–127 | 100 |
| `rotation` | Integer -4096–4096 | 0 |
| `rootKey` | Integer -1–127 | -1 |
| `drumVelocityBits` | Integer 1–7 | 1 |
| `gate` | Finite decimal number 0–2 | 0.5 |
| `fixedLengthSteps` | Finite decimal number 0–16 | 0 |
| `reverse`, `favourite` | `0`/`1`, `false`/`true`, or `no`/`yes` (case insensitive) | false |
| `name` | Up to 80 characters | `Pattern N`, where N is the entry number |
| `tags` | Up to 200 characters | Empty |
| `colour` | `#` followed by six hexadecimal digits | `#62D6C6` |
| `forte` | Valid Forte set ID, up to 64 characters, or empty | Empty |
| `drumNotes` | 1–16 MIDI note numbers | `36 38 42 46 41 43 45 47 48 50 49 51 39 37 54 56` |

Integer values must contain a single whole number. Decimal numbers use a dot
and may use scientific notation. Invalid optional attributes are rejected;
they are not silently clamped, truncated, or replaced with defaults. Empty
names receive the same fallback as omitted names. XML attribute values must
escape special characters such as `&` and `<` using standard XML entities.

The importer accepts files up to 16 MiB and at most 2000 patterns. It rejects
malformed XML, unsupported versions, duplicate IDs, invalid sequences and
invalid settings before it writes any patterns. Export uses the same count,
size and validation limits so its files can be reimported. Local entries with
missing or duplicate IDs must be repaired before exporting; export reports
an error instead of creating an unusable bank file.
