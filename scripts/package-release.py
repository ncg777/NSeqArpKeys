"""Package a JUCE CMake build for one operating system."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import zipfile


ROOT = Path(__file__).resolve().parents[1]
VERSION = re.search(r"project\(NSeqArpKeys VERSION (\d+\.\d+\.\d+)",
                    (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")).group(1)


def add_file(archive, source, entry, sums):
    data = source.read_bytes()
    if not data:
        raise ValueError(f"Empty release file: {source}")
    if entry.endswith("moduleinfo.json"):
        # JUCE 8.0.6 may leave a trailing comma in the VST3 manifest.
        data = re.sub(rb",(?=\s*[}\]])", b"", data)
        metadata = json.loads(data)
        if metadata["Name"] != "NSeqArpKeys" or metadata["Version"] != VERSION:
            raise ValueError(f"Unexpected VST3 manifest in {source}")
    info = zipfile.ZipInfo(entry)
    info.compress_type = zipfile.ZIP_DEFLATED
    info.external_attr = (stat.S_IFREG | source.stat().st_mode & 0o777) << 16
    archive.writestr(info, data)
    sums.append(f"{hashlib.sha256(data).hexdigest()}  {entry}")


def add_tree(archive, source, root_name, sums):
    if not source.is_dir():
        raise FileNotFoundError(source)
    files = 0
    for path in sorted(source.rglob("*")):
        entry = (Path(root_name) / path.relative_to(source)).as_posix()
        if path.is_symlink():
            info = zipfile.ZipInfo(entry)
            info.external_attr = (stat.S_IFLNK | 0o777) << 16
            archive.writestr(info, os.readlink(path).encode("utf-8"))
        elif path.is_file():
            add_file(archive, path, entry, sums)
            files += 1
    if files == 0:
        raise ValueError(f"Empty release bundle: {source}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--platform", choices=("windows-x64", "linux-x64", "macos-universal"),
                        required=True)
    args = parser.parse_args()

    artifacts = args.build_dir / "NSeqArpKeys_artefacts" / "Release"
    output = ROOT / "release" / f"NSeqArpKeys-{VERSION}-{args.platform}.zip"
    output.parent.mkdir(parents=True, exist_ok=True)
    manual = ROOT / "output" / "pdf" / "NSeqArpKeys-Manual.pdf"
    if not manual.is_file():
        raise FileNotFoundError(manual)
    manifest = artifacts / "VST3" / "NSeqArpKeys.vst3" / "Contents" / "Resources" / "moduleinfo.json"
    if not manifest.is_file():
        raise FileNotFoundError(manifest)

    sums = []
    with zipfile.ZipFile(output, "w") as archive:
        if args.platform == "windows-x64":
            add_file(archive, artifacts / "Standalone" / "NSeqArpKeys.exe",
                     "NSeqArpKeys.exe", sums)
        elif args.platform == "linux-x64":
            add_file(archive, artifacts / "Standalone" / "NSeqArpKeys",
                     "NSeqArpKeys", sums)
        else:
            add_tree(archive, artifacts / "Standalone" / "NSeqArpKeys.app",
                     "NSeqArpKeys.app", sums)
            add_tree(archive, artifacts / "AU" / "NSeqArpKeys.component",
                     "NSeqArpKeys.component", sums)
        add_tree(archive, artifacts / "VST3" / "NSeqArpKeys.vst3", "NSeqArpKeys.vst3", sums)
        add_file(archive, manual, "NSeqArpKeys-Manual.pdf", sums)
        archive.writestr("SHA256SUMS.txt", "\n".join(sums) + "\n")

    with zipfile.ZipFile(output) as archive:
        if archive.testzip() is not None:
            raise ValueError(f"ZIP integrity check failed: {output}")
    print(output)


if __name__ == "__main__":
    main()
