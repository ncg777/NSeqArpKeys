# NSeqArpKeys 1.3.1

## Pattern Bank transfer

- The Pattern Bank now imports and exports all entries in a single XML-based
  `.nseqbank` file. The file preserves the pattern ID, assignment, name, tags,
  colour and favourite state of each entry.
- Import validates the complete file before changing the local bank. Entries
  with matching IDs update the corresponding local pattern; new IDs add
  patterns. Other local patterns remain in place. If writing fails, the status
  reports how many entries were imported before the failure.
- Export includes the whole bank, regardless of search text or the favourites
  filter. An empty bank can also be exported.
- Externally generated files may omit pattern IDs. The importer assigns an ID
  to each such entry. The [file format](pattern-bank-format.md) documents the
  structure and limits.

Individual `.nseqpattern` files and whole-preset `.nseqpreset` files continue to
work as before. The 1.3.1 plug-in keeps the same host parameters and project
state format as 1.3.0.
