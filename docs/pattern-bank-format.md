# Pattern Bank file format

A `.nseqbank` file is UTF-8 XML. Its root is `NSeqPatternBank` with a `version`
attribute of `1`. Each direct child is an `NSeqPattern` with `version="1"` and
one `Assignment` child. These `NSeqPattern` elements use the same structure as
individual `.nseqpattern` files.

The optional `id` attribute on `NSeqPattern` is a stable identifier of at most
80 characters. IDs must be unique within a bank file. Importing an ID that is
already in the local bank updates that entry; a new ID adds an entry. When `id`
is omitted, the importer generates one and adds a new entry. Export always
includes IDs. Bank import does not delete local entries absent from the file.

`Assignment` must have a `sequence` attribute containing up to 4096
space-separated signed decimal steps (or positive decimal rhythmic masks up
to 112 bits). It may be empty. Other supported attributes are `mode`
(`melodic` or `rhythmic`), `name`, `tags`, `colour`, `favourite`, `forte`,
`channel`, `octave`, `gate`, `fixedLengthSteps`, `subdivision`, `transpose`,
`velocity`, `rotation`, `reverse`, `rootKey`, `drumNotes`, and
`drumVelocityBits`. The optional `drumNotes` attribute contains 1–16
space-separated MIDI note numbers from 0–127. Omitted optional attributes use
the same defaults as individual pattern files. The `linkId` attribute, if
present, is ignored during bank import because project links are kept in the
project state.

The importer accepts files up to 16 MiB and at most 2000 patterns. It rejects
malformed XML, unsupported versions, duplicate IDs, invalid sequences and
invalid drum notes before it writes any patterns. Export uses the same size
limit so its files can be reimported.
