#!/usr/bin/env python3
import gzip
import sys
from pathlib import Path


def gzip_bytes(data: bytes) -> bytes:
    # Deterministic gzip (mtime=0) for reproducible builds.
    import io

    bio = io.BytesIO()
    with gzip.GzipFile(fileobj=bio, mode="wb", mtime=0) as gz:
        gz.write(data)
    return bio.getvalue()


def main() -> int:
    if len(sys.argv) != 3:
        print("Usage: compress_assets.py <ResourcesDir> <OutDir>", file=sys.stderr)
        return 2

    resources_dir = Path(sys.argv[1]).resolve()
    out_dir = Path(sys.argv[2]).resolve()
    midi_files = sorted((resources_dir / "MIDI").rglob("*.mid"))
    json_files = sorted((resources_dir / "Presets").rglob("*.json"))

    if len(midi_files) != 500 or len(json_files) != 200:
        raise RuntimeError(f"Expected 500 MIDI + 200 JSON in Resources; got {len(midi_files)} + {len(json_files)}")

    for src in midi_files + json_files:
        rel = src.relative_to(resources_dir)
        dst = out_dir / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        dst.write_bytes(gzip_bytes(src.read_bytes()))

    # Sanity counts
    out_midi = len(list((out_dir / "MIDI").rglob("*.mid")))
    out_json = len(list((out_dir / "Presets").rglob("*.json")))
    if out_midi != 500 or out_json != 200:
        raise RuntimeError(f"Compressed counts mismatch: midi={out_midi}, json={out_json}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
