"""Build the deterministic Fourth Atlas archive embedded in the plug-in."""

from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo


ROOT = Path(__file__).resolve().parents[1]
BANKS = ROOT / "banks" / "FourthAtlas"
OUTPUT = BANKS / "FourthAtlas.zip"
EXPECTED_NAMES = (
    "00-START-HERE.nseqbank",
    "01-filigree-melodic.nseqbank", "02-filigree-rhythmic.nseqbank",
    "03-antiphon-melodic.nseqbank", "04-antiphon-rhythmic.nseqbank",
    "05-embers-melodic.nseqbank", "06-embers-rhythmic.nseqbank",
    "07-kaleidoscope-melodic.nseqbank", "08-kaleidoscope-rhythmic.nseqbank",
    "09-lattice-melodic.nseqbank", "10-lattice-rhythmic.nseqbank",
    "11-orbit-melodic.nseqbank", "12-orbit-rhythmic.nseqbank",
)


def main() -> None:
    sources = sorted(BANKS.glob("*.nseqbank"))
    if tuple(source.name for source in sources) != EXPECTED_NAMES:
        raise ValueError("Fourth Atlas bank files do not match the built-in selector")
    with ZipFile(OUTPUT, "w") as archive:
        for source in sources:
            info = ZipInfo(source.name, date_time=(2026, 9, 25, 0, 0, 0))
            info.compress_type = ZIP_DEFLATED
            archive.writestr(info, source.read_bytes(), compress_type=ZIP_DEFLATED,
                             compresslevel=9)
    with ZipFile(OUTPUT) as archive:
        if archive.testzip() is not None:
            raise ValueError("Built-in bank archive failed integrity check")
    print(OUTPUT)


if __name__ == "__main__":
    main()
