"""Refresh the manual's Forte table from the plug-in's own catalogue."""

from __future__ import annotations

import argparse
import html
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CATALOGUE = ROOT / "Source" / "ForteCSV.cpp"
MANUAL = ROOT / "docs" / "NSeqArpKeys-Manual.html"
START = "<!-- FORTE_TABLE_START -->"
END = "<!-- FORTE_TABLE_END -->"


def catalogue_rows() -> list[tuple[str, tuple[int, ...]]]:
    source = CATALOGUE.read_text(encoding="utf-8")
    block = source.split('ForteCSV::FORTE_NUMBERS = R"(', 1)[1].split(')";', 1)[0]
    rows = [
        (name, tuple(int(value) for value in pitches.split()))
        for name, pitches in re.findall(r'"([^"]+)","([^"]*)"', block)
    ]
    assert len(rows) == 352 and len({name for name, _ in rows}) == len(rows)
    for name, pitches in rows:
        assert len(pitches) == int(name.split("-", 1)[0])
        assert tuple(sorted(set(pitches))) == pitches
        assert all(0 <= pitch < 12 for pitch in pitches)
    distinct_transpositions = {
        tuple(sorted((pitch + shift) % 12 for pitch in pitches))
        for _, pitches in rows
        for shift in range(12)
    }
    assert len(distinct_transpositions) == 4096
    return sorted(rows, key=lambda row: sort_key(row[0]))


def sort_key(name: str) -> tuple[int, int, str, str]:
    match = re.fullmatch(r"(\d+)-(z?)(\d+)([AB]?)", name)
    assert match is not None, name
    cardinality, z, number, ab = match.groups()
    return int(cardinality), int(number), z, ab


def interval_vector(pitches: tuple[int, ...]) -> tuple[int, ...]:
    counts = [0] * 6
    for index, first in enumerate(pitches):
        for second in pitches[index + 1 :]:
            distance = (second - first) % 12
            counts[min(distance, 12 - distance) - 1] += 1
    assert sum(counts) == len(pitches) * (len(pitches) - 1) // 2
    return tuple(counts)


def table_html() -> str:
    lines = [
        '<table class="forte-table">',
        '  <thead><tr><th>Forte ID (.0)</th><th>Pitch classes</th><th>Interval vector &lt;1 2 3 4 5 6&gt;</th></tr></thead>',
        "  <tbody>",
    ]
    for name, pitches in catalogue_rows():
        pitch_text = "[" + " ".join(map(str, pitches)) + "]"
        vector_text = "&lt;" + " ".join(map(str, interval_vector(pitches))) + "&gt;"
        lines.append(
            "    <tr><td>"
            + html.escape(name)
            + ".0</td><td>"
            + pitch_text
            + "</td><td>"
            + vector_text
            + "</td></tr>"
        )
    lines += ["  </tbody>", "</table>"]
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Check that the manual table is current")
    args = parser.parse_args()

    source = MANUAL.read_text(encoding="utf-8")
    assert source.count(START) == source.count(END) == 1
    before, remainder = source.split(START, 1)
    _, after = remainder.split(END, 1)
    updated = before + START + "\n" + table_html() + "\n" + END + after
    if args.check:
        if source != updated:
            raise SystemExit("The manual Forte appendix needs regeneration")
        print("Forte appendix is current: 352 base sets")
    else:
        MANUAL.write_text(updated, encoding="utf-8")
        print("Updated Forte appendix: 352 base sets")


if __name__ == "__main__":
    main()
