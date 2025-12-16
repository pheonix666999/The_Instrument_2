#!/usr/bin/env python3
import os
import random
import struct
import sys
from pathlib import Path


GENRES = [
    "House", "Deep House", "Tech House", "Progressive House",
    "Future House", "Bass House", "Electro House", "Big Room",
    "Techno", "Melodic Techno", "Minimal", "Trance",
    "Progressive Trance", "Hardstyle", "Dubstep", "Future Bass",
    "Drum & Bass", "Liquid DnB", "Garage", "UK Bass",
    "Trap", "Hip Hop", "R&B", "Pop",
    "Synthwave", "Disco", "Funk", "Ambient",
    "Lo-Fi", "EDM", "Tropical House", "Afro House",
]


def vlq(value: int) -> bytes:
    """Variable-length quantity for MIDI."""
    if value < 0:
        value = 0
    buffer = value & 0x7F
    out = bytearray()
    while True:
        value >>= 7
        if value:
            buffer <<= 8
            buffer |= ((value & 0x7F) | 0x80)
        else:
            break
    while True:
        out.append(buffer & 0xFF)
        if buffer & 0x80:
            buffer >>= 8
        else:
            break
    return bytes(out)


def midi_note_on(channel: int, note: int, vel: int) -> bytes:
    return bytes([0x90 | (channel & 0x0F), note & 0x7F, vel & 0x7F])


def midi_note_off(channel: int, note: int, vel: int = 0) -> bytes:
    return bytes([0x80 | (channel & 0x0F), note & 0x7F, vel & 0x7F])


def midi_program_change(channel: int, program: int) -> bytes:
    return bytes([0xC0 | (channel & 0x0F), program & 0x7F])


def meta_tempo(bpm: float) -> bytes:
    mpqn = int(60_000_000 / max(1.0, bpm))
    return bytes([0xFF, 0x51, 0x03]) + struct.pack(">I", mpqn)[1:]


def meta_time_signature(numer: int = 4, denom: int = 4) -> bytes:
    dd = {1: 0, 2: 1, 4: 2, 8: 3, 16: 4}.get(denom, 2)
    return bytes([0xFF, 0x58, 0x04, numer & 0xFF, dd & 0xFF, 24, 8])


def meta_end_of_track() -> bytes:
    return bytes([0xFF, 0x2F, 0x00])


def make_midi_single_track(events: list[tuple[int, bytes]], tpq: int = 960) -> bytes:
    track_data = bytearray()
    last_tick = 0
    for tick, data in sorted(events, key=lambda x: x[0]):
        dt = max(0, tick - last_tick)
        track_data += vlq(dt)
        track_data += data
        last_tick = tick

    track_data += vlq(0) + meta_end_of_track()

    header = b"MThd" + struct.pack(">IHHH", 6, 1, 1, tpq)
    track = b"MTrk" + struct.pack(">I", len(track_data)) + bytes(track_data)
    return header + track


def generate_chord_progression_midi(seed: int, bars: int) -> bytes:
    rnd = random.Random(seed)
    tpq = 960
    ticks_per_16 = tpq // 4
    total_ticks = bars * 4 * tpq
    base = rnd.choice([36, 48, 60])  # bass, mid, piano-ish
    channel = 0

    events = []
    events.append((0, meta_time_signature(4, 4)))
    events.append((0, meta_tempo(rnd.choice([120.0, 124.0, 126.0, 128.0]))))
    events.append((0, midi_program_change(channel, rnd.randrange(0, 32))))

    # Simple four-chord pattern: I - V - vi - IV (encoded as root notes)
    degrees = [0, 7, 9, 5]
    for bar in range(bars):
        chord_root = base + degrees[(bar // 2) % len(degrees)]
        for beat in range(4):
            # Some genres hit chords on 1 and 3, others every beat
            if rnd.random() < 0.55 and beat in (1, 3):
                continue
            start = (bar * 4 + beat) * tpq
            dur = rnd.choice([tpq, tpq * 2, tpq // 2])
            dur = min(dur, total_ticks - start - ticks_per_16)
            vel = rnd.randrange(60, 105)
            events.append((start, midi_note_on(channel, chord_root, vel)))
            events.append((start + dur, midi_note_off(channel, chord_root, 0)))

    return make_midi_single_track(events, tpq=tpq)


def generate_melody_oneshot_midi(seed: int, bars: int) -> bytes:
    rnd = random.Random(seed)
    tpq = 960
    channel = 0
    base = rnd.choice([60, 72])
    scale = [0, 2, 3, 5, 7, 8, 10]  # minor-ish
    events = []
    events.append((0, meta_time_signature(4, 4)))
    events.append((0, meta_tempo(rnd.choice([120.0, 124.0, 128.0, 132.0]))))

    t = 0
    total = bars * 4 * tpq
    last_note = base + rnd.choice(scale)
    while t < total:
        step = rnd.choice([-2, -1, 0, 1, 2])
        idx = (scale.index((last_note - base) % 12) + step) % len(scale)
        note = base + scale[idx] + rnd.choice([-12, 0, 12]) * (1 if rnd.random() < 0.2 else 0)
        note = max(36, min(96, note))
        dur = rnd.choice([tpq // 4, tpq // 2, tpq])
        vel = rnd.randrange(55, 110)
        events.append((t, midi_note_on(channel, note, vel)))
        events.append((t + dur, midi_note_off(channel, note, 0)))
        t += rnd.choice([tpq // 4, tpq // 2, tpq // 8])
        last_note = note

    return make_midi_single_track(events, tpq=tpq)


def generate_preset_json(index: int) -> str:
    # 10 base presets repeated/patterned to 200.
    base_presets = [
        ("House Pluck", {"osc1": "saw", "osc2": "square", "detune": 0.08, "cutoff": 0.65, "res": 0.18}),
        ("Deep Pad", {"osc1": "saw", "osc2": "saw", "detune": 0.14, "cutoff": 0.45, "res": 0.10}),
        ("Tech Bass", {"osc1": "square", "osc2": "saw", "detune": 0.04, "cutoff": 0.35, "res": 0.22}),
        ("Future Lead", {"osc1": "saw", "osc2": "square", "detune": 0.11, "cutoff": 0.58, "res": 0.12}),
        ("Disco Keys", {"osc1": "square", "osc2": "square", "detune": 0.02, "cutoff": 0.72, "res": 0.08}),
        ("Lo-Fi Bell", {"osc1": "saw", "osc2": "saw", "detune": 0.06, "cutoff": 0.40, "res": 0.05}),
        ("Ambient Wash", {"osc1": "saw", "osc2": "saw", "detune": 0.17, "cutoff": 0.33, "res": 0.07}),
        ("Trap Pluck", {"osc1": "square", "osc2": "square", "detune": 0.03, "cutoff": 0.62, "res": 0.14}),
        ("Melodic Techno Stab", {"osc1": "saw", "osc2": "square", "detune": 0.09, "cutoff": 0.52, "res": 0.20}),
        ("Synthwave Poly", {"osc1": "saw", "osc2": "square", "detune": 0.12, "cutoff": 0.68, "res": 0.09}),
    ]
    name, d = base_presets[index % len(base_presets)]
    genre = GENRES[index % len(GENRES)]
    macro = (index * 37) % 128
    d2 = dict(d)
    d2["name"] = f"{name} {index:03d}"
    d2["genre"] = genre
    d2["macro_hint"] = macro
    # 13 macros: store defaults 0..1
    d2["macros"] = [round(((index + i * 11) % 128) / 127.0, 4) for i in range(13)]
    import json
    return json.dumps(d2, indent=2, sort_keys=True)


def write_file(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)


def main():
    if len(sys.argv) != 2:
        print("Usage: generate_assets.py <ResourcesDir>", file=sys.stderr)
        return 2

    resources_dir = Path(sys.argv[1]).resolve()
    midi_dir = resources_dir / "MIDI"
    presets_dir = resources_dir / "Presets"

    chord_dir = midi_dir / "ChordProgressions"
    melody_dir = midi_dir / "MelodyOneShots"

    chord_dir.mkdir(parents=True, exist_ok=True)
    melody_dir.mkdir(parents=True, exist_ok=True)
    presets_dir.mkdir(parents=True, exist_ok=True)

    # Deterministic, stable generation.
    # Chords: 300 files, each 4-8 bars
    for i in range(300):
        bars = 4 + (i % 5)  # 4..8
        data = generate_chord_progression_midi(seed=1000 + i, bars=bars)
        write_file(chord_dir / f"ChordProg_{i:03d}.mid", data)

    # Melody one-shots: 200 files, each 1-4 bars
    for i in range(200):
        bars = 1 + (i % 4)  # 1..4
        data = generate_melody_oneshot_midi(seed=5000 + i, bars=bars)
        write_file(melody_dir / f"MelodyShot_{i:03d}.mid", data)

    # Presets: 200 json
    for i in range(200):
        (presets_dir / f"Preset_{i:03d}.json").write_text(generate_preset_json(i), encoding="utf-8")

    # Sanity counts
    midi_count = len(list(midi_dir.rglob("*.mid")))
    json_count = len(list(presets_dir.rglob("*.json")))
    if midi_count != 500 or json_count != 200:
        raise RuntimeError(f"Generated wrong counts: midi={midi_count}, json={json_count}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
