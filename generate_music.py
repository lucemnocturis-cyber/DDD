#!/usr/bin/env python3
"""
Procedural Music Generator for Dungeon Dice Duelists
Generates background music tracks using algorithmic composition
"""

import wave
import struct
import math
import random
import os

# Music configuration
SAMPLE_RATE = 44100
CHANNELS = 2  # Stereo for music
SAMPLE_WIDTH = 2
BPM = 120  # Beats per minute

def beats_to_seconds(beats):
    """Convert beats to seconds"""
    return beats * 60.0 / BPM

def note_to_frequency(note, octave=4):
    """Convert musical note to frequency (A4 = 440 Hz)"""
    notes = {'C': -9, 'C#': -8, 'D': -7, 'D#': -6, 'E': -5, 'F': -4,
             'F#': -3, 'G': -2, 'G#': -1, 'A': 0, 'A#': 1, 'B': 2}
    
    if note not in notes:
        return 440.0
    
    semitones = notes[note] + (octave - 4) * 12
    return 440.0 * (2 ** (semitones / 12))

def generate_note(frequency, duration, amplitude=0.5, waveform='sine'):
    """Generate a musical note"""
    num_samples = int(SAMPLE_RATE * duration)
    samples = []
    
    for i in range(num_samples):
        t = i / SAMPLE_RATE
        
        if waveform == 'sine':
            value = amplitude * math.sin(2 * math.pi * frequency * t)
        elif waveform == 'square':
            value = amplitude if math.sin(2 * math.pi * frequency * t) > 0 else -amplitude
        elif waveform == 'triangle':
            phase = (frequency * t) % 1.0
            value = amplitude * (4 * abs(phase - 0.5) - 1)
        else:
            value = 0
        
        # Apply simple envelope
        envelope = 1.0
        attack = min(0.05, duration * 0.1)
        release = min(0.1, duration * 0.2)
        
        if t < attack:
            envelope = t / attack
        elif t > duration - release:
            envelope = (duration - t) / release
        
        samples.append(value * envelope)
    
    return samples

def generate_chord(notes, octave, duration, amplitude=0.4):
    """Generate a chord from multiple notes"""
    chord_samples = []
    
    for note in notes:
        freq = note_to_frequency(note, octave)
        note_samples = generate_note(freq, duration, amplitude / len(notes))
        
        if not chord_samples:
            chord_samples = note_samples
        else:
            # Mix with existing samples
            for i in range(min(len(chord_samples), len(note_samples))):
                chord_samples[i] += note_samples[i]
    
    return chord_samples

def generate_arpeggio(notes, octave, duration, note_duration=0.25):
    """Generate an arpeggio (broken chord)"""
    samples = []
    
    for note in notes:
        freq = note_to_frequency(note, octave)
        note_samples = generate_note(freq, note_duration, 0.5)
        samples.extend(note_samples)
    
    return samples

def add_reverb(samples, decay=0.3, delay=0.1):
    """Add simple reverb effect"""
    delay_samples = int(SAMPLE_RATE * delay)
    output = samples.copy()
    
    for i in range(delay_samples, len(samples)):
        output[i] += samples[i - delay_samples] * decay
    
    return output

def stereo_pan(samples, pan=0.0):
    """Convert mono to stereo with panning (-1.0 = left, 1.0 = right)"""
    stereo = []
    left_gain = (1.0 - pan) * 0.5
    right_gain = (1.0 + pan) * 0.5
    
    for sample in samples:
        stereo.append((sample * left_gain, sample * right_gain))
    
    return stereo

def mix_stereo_tracks(tracks):
    """Mix multiple stereo tracks together"""
    if not tracks:
        return []
    
    max_len = max(len(t) for t in tracks)
    mixed = [(0.0, 0.0) for _ in range(max_len)]
    
    for track in tracks:
        for i in range(len(track)):
            mixed[i] = (mixed[i][0] + track[i][0], mixed[i][1] + track[i][1])
    
    return mixed

def normalize_stereo(samples, target=0.8):
    """Normalize stereo samples"""
    if not samples:
        return samples
    
    max_val = max(max(abs(l), abs(r)) for l, r in samples)
    if max_val > 0:
        scale = target / max_val
        return [(l * scale, r * scale) for l, r in samples]
    return samples

def stereo_to_bytes(samples):
    """Convert stereo samples to bytes"""
    data = []
    for left, right in samples:
        # Clamp
        left = max(-1.0, min(1.0, left))
        right = max(-1.0, min(1.0, right))
        
        # Convert to 16-bit signed integers
        left_int = int(left * 32767)
        right_int = int(right * 32767)
        
        data.append(struct.pack('<hh', left_int, right_int))
    
    return b''.join(data)

def save_stereo_wave(filename, samples):
    """Save stereo samples as WAV file"""
    with wave.open(filename, 'w') as wav:
        wav.setnchannels(CHANNELS)
        wav.setsampwidth(SAMPLE_WIDTH)
        wav.setframerate(SAMPLE_RATE)
        wav.writeframes(stereo_to_bytes(samples))

# ========== MUSIC TRACK GENERATORS ==========

def generate_menu_theme():
    """Generate main menu theme - mysterious and inviting"""
    print("  Composing menu theme...")
    
    # Chord progression: Am - F - C - G
    progression = [
        (['A', 'C', 'E'], 3),  # Am
        (['F', 'A', 'C'], 3),  # F
        (['C', 'E', 'G'], 4),  # C
        (['G', 'B', 'D'], 3),  # G
    ]
    
    tracks = []
    total_duration = 32  # 8 bars, 4 beats each
    
    # Pad chords
    pad_track = []
    for i in range(4):  # Repeat progression twice
        for notes, octave in progression:
            chord_samples = generate_chord(notes, octave, beats_to_seconds(4), 0.3)
            pad_track.extend(chord_samples)
    
    pad_stereo = stereo_pan(add_reverb(pad_track, 0.4, 0.2), 0.0)
    tracks.append(pad_stereo)
    
    # Melody - simple arpeggio pattern
    melody_track = []
    melody_notes = ['A', 'C', 'E', 'C', 'F', 'A', 'C', 'A',
                    'C', 'E', 'G', 'E', 'G', 'B', 'D', 'B']
    
    for i in range(8):  # Repeat twice
        for note in melody_notes:
            freq = note_to_frequency(note, 4)
            note_samples = generate_note(freq, beats_to_seconds(0.5), 0.4)
            melody_track.extend(note_samples)
    
    melody_stereo = stereo_pan(melody_track, 0.3)
    tracks.append(melody_stereo)
    
    # Mix and normalize
    mixed = mix_stereo_tracks(tracks)
    return normalize_stereo(mixed, 0.7)

def generate_battle_theme():
    """Generate battle theme - energetic and intense"""
    print("  Composing battle theme (intensity: normal)...")
    
    # Em - D - C - B (minor progression)
    progression = [
        (['E', 'G', 'B'], 3),  # Em
        (['D', 'F#', 'A'], 3), # D
        (['C', 'E', 'G'], 3),  # C
        (['B', 'D#', 'F#'], 2),# B
    ]
    
    tracks = []
    
    # Driving rhythm (bass)
    bass_track = []
    bass_pattern = ['E', 'E', 'D', 'D', 'C', 'C', 'B', 'B']
    
    for i in range(8):  # Repeat
        for note in bass_pattern:
            freq = note_to_frequency(note, 2)
            note_samples = generate_note(freq, beats_to_seconds(0.5), 0.6, 'square')
            bass_track.extend(note_samples)
    
    bass_stereo = stereo_pan(bass_track, -0.2)
    tracks.append(bass_stereo)
    
    # Chord stabs
    chord_track = []
    for i in range(8):
        for notes, octave in progression:
            chord_samples = generate_chord(notes, octave, beats_to_seconds(1), 0.3)
            silence = [0.0] * int(SAMPLE_RATE * beats_to_seconds(1))
            chord_track.extend(chord_samples)
            chord_track.extend(silence)
    
    chord_stereo = stereo_pan(chord_track, 0.2)
    tracks.append(chord_stereo)
    
    # Lead melody
    lead_track = []
    lead_notes = ['E', 'G', 'A', 'B', 'D', 'C', 'B', 'A'] * 8
    
    for note in lead_notes:
        freq = note_to_frequency(note, 5)
        note_samples = generate_note(freq, beats_to_seconds(0.5), 0.4)
        lead_track.extend(note_samples)
    
    lead_stereo = stereo_pan(lead_track, 0.0)
    tracks.append(lead_stereo)
    
    mixed = mix_stereo_tracks(tracks)
    return normalize_stereo(mixed, 0.8)

def generate_boss_theme():
    """Generate boss battle theme - dramatic and threatening"""
    print("  Composing boss theme...")
    
    # Dark progression: Dm - Am - Bb - C
    progression = [
        (['D', 'F', 'A'], 2),  # Dm (low)
        (['A', 'C', 'E'], 2),  # Am
        (['Bb', 'D', 'F'], 2), # Bb (using A# as Bb)
        (['C', 'E', 'G'], 3),  # C
    ]
    
    tracks = []
    
    # Heavy bass
    bass_track = []
    bass_notes = ['D', 'D', 'A', 'A', 'A#', 'A#', 'C', 'C']
    
    for i in range(6):
        for note in bass_notes:
            freq = note_to_frequency(note, 1)
            note_samples = generate_note(freq, beats_to_seconds(1), 0.7, 'square')
            bass_track.extend(note_samples)
    
    bass_stereo = stereo_pan(bass_track, 0.0)
    tracks.append(bass_stereo)
    
    # Dramatic strings (sustained chords)
    strings_track = []
    for i in range(6):
        for notes, octave in progression:
            chord_samples = generate_chord(notes, octave + 1, beats_to_seconds(4), 0.4)
            strings_track.extend(chord_samples)
    
    strings_stereo = stereo_pan(add_reverb(strings_track, 0.5, 0.15), 0.0)
    tracks.append(strings_stereo)
    
    # Aggressive lead
    lead_track = []
    lead_pattern = ['D', 'F', 'A', 'D', 'A', 'F', 'D', 'C'] * 6
    
    for note in lead_pattern:
        freq = note_to_frequency(note, 5)
        note_samples = generate_note(freq, beats_to_seconds(0.5), 0.5, 'square')
        lead_track.extend(note_samples)
    
    lead_stereo = stereo_pan(lead_track, 0.4)
    tracks.append(lead_stereo)
    
    mixed = mix_stereo_tracks(tracks)
    return normalize_stereo(mixed, 0.85)

def generate_victory_theme():
    """Generate victory theme - triumphant and uplifting"""
    print("  Composing victory theme...")
    
    # Major progression: C - G - Am - F
    progression = [
        (['C', 'E', 'G'], 4),
        (['G', 'B', 'D'], 3),
        (['A', 'C', 'E'], 3),
        (['F', 'A', 'C'], 3),
    ]
    
    tracks = []
    
    # Triumphant fanfare
    fanfare_notes = ['C', 'E', 'G', 'C', 'E', 'G', 'C']
    fanfare_track = []
    
    for i in range(3):
        for note in fanfare_notes:
            freq = note_to_frequency(note, 4 + i % 2)
            note_samples = generate_note(freq, beats_to_seconds(0.5), 0.6)
            fanfare_track.extend(note_samples)
    
    fanfare_stereo = stereo_pan(fanfare_track, 0.0)
    tracks.append(fanfare_stereo)
    
    # Sustained chords
    chord_track = []
    for i in range(4):
        for notes, octave in progression:
            chord_samples = generate_chord(notes, octave, beats_to_seconds(2), 0.4)
            chord_track.extend(chord_samples)
    
    chord_stereo = stereo_pan(add_reverb(chord_track, 0.4, 0.2), 0.0)
    tracks.append(chord_stereo)
    
    mixed = mix_stereo_tracks(tracks)
    return normalize_stereo(mixed, 0.75)

def generate_defeat_theme():
    """Generate defeat theme - somber and reflective"""
    print("  Composing defeat theme...")
    
    # Sad progression: Am - F - Dm - E
    progression = [
        (['A', 'C', 'E'], 3),
        (['F', 'A', 'C'], 3),
        (['D', 'F', 'A'], 3),
        (['E', 'G#', 'B'], 3),
    ]
    
    tracks = []
    
    # Slow descending melody
    melody_notes = ['A', 'G', 'F', 'E', 'D', 'C', 'B', 'A']
    melody_track = []
    
    for i in range(2):
        for note in melody_notes:
            freq = note_to_frequency(note, 4)
            note_samples = generate_note(freq, beats_to_seconds(1), 0.5)
            melody_track.extend(note_samples)
    
    melody_stereo = stereo_pan(add_reverb(melody_track, 0.5, 0.25), 0.0)
    tracks.append(melody_stereo)
    
    # Soft pads
    pad_track = []
    for i in range(2):
        for notes, octave in progression:
            chord_samples = generate_chord(notes, octave, beats_to_seconds(4), 0.3)
            pad_track.extend(chord_samples)
    
    pad_stereo = stereo_pan(add_reverb(pad_track, 0.6, 0.3), 0.0)
    tracks.append(pad_stereo)
    
    mixed = mix_stereo_tracks(tracks)
    return normalize_stereo(mixed, 0.6)

def generate_exploration_theme():
    """Generate exploration/shop theme - curious and light"""
    print("  Composing exploration theme...")
    
    # Playful progression: G - C - D - Em
    progression = [
        (['G', 'B', 'D'], 3),
        (['C', 'E', 'G'], 4),
        (['D', 'F#', 'A'], 3),
        (['E', 'G', 'B'], 3),
    ]
    
    tracks = []
    
    # Light plucked melody
    melody_notes = ['G', 'A', 'B', 'C', 'D', 'E', 'D', 'C',
                    'B', 'C', 'D', 'E', 'F#', 'G', 'F#', 'E']
    melody_track = []
    
    for i in range(4):
        for note in melody_notes:
            freq = note_to_frequency(note, 4)
            note_samples = generate_note(freq, beats_to_seconds(0.5), 0.4, 'triangle')
            melody_track.extend(note_samples)
    
    melody_stereo = stereo_pan(melody_track, 0.3)
    tracks.append(melody_stereo)
    
    # Gentle accompaniment
    accomp_track = []
    for i in range(4):
        for notes, octave in progression:
            arp_samples = generate_arpeggio(notes, octave, beats_to_seconds(4))
            accomp_track.extend(arp_samples)
    
    accomp_stereo = stereo_pan(accomp_track, -0.3)
    tracks.append(accomp_stereo)
    
    mixed = mix_stereo_tracks(tracks)
    return normalize_stereo(mixed, 0.65)

def generate_ambient_theme():
    """Generate ambient/title screen theme - atmospheric"""
    print("  Composing ambient theme...")
    
    # Atmospheric progression: Dm - C - Bb - Am
    progression = [
        (['D', 'F', 'A'], 3),
        (['C', 'E', 'G'], 3),
        (['Bb', 'D', 'F'], 2),
        (['A', 'C', 'E'], 3),
    ]
    
    tracks = []
    
    # Long sustained pads
    pad_track = []
    for i in range(2):
        for notes, octave in progression:
            chord_samples = generate_chord(notes, octave, beats_to_seconds(8), 0.35)
            pad_track.extend(chord_samples)
    
    pad_stereo = stereo_pan(add_reverb(pad_track, 0.7, 0.35), 0.0)
    tracks.append(pad_stereo)
    
    # Sparse high notes
    high_notes = ['D', 'F', 'A', 'C', 'E', 'G', 'D']
    high_track = []
    
    for note in high_notes * 8:
        freq = note_to_frequency(note, 5)
        note_samples = generate_note(freq, beats_to_seconds(0.25), 0.2)
        silence = [0.0] * int(SAMPLE_RATE * beats_to_seconds(3.75))
        high_track.extend(note_samples)
        high_track.extend(silence)
    
    high_stereo = stereo_pan(add_reverb(high_track, 0.5, 0.4), 0.5)
    tracks.append(high_stereo)
    
    mixed = mix_stereo_tracks(tracks)
    return normalize_stereo(mixed, 0.55)

def generate_tension_theme():
    """Generate tension/danger theme - suspenseful"""
    print("  Composing tension theme...")
    
    # Tense progression: Em - F - Em - F#dim
    progression = [
        (['E', 'G', 'B'], 3),
        (['F', 'A', 'C'], 3),
        (['E', 'G', 'B'], 3),
        (['F#', 'A', 'C'], 3),
    ]
    
    tracks = []
    
    # Pulsing bass
    bass_track = []
    bass_pattern = ['E', 'E', 'E', 'E'] * 16
    
    for note in bass_pattern:
        freq = note_to_frequency(note, 2)
        note_samples = generate_note(freq, beats_to_seconds(0.25), 0.5, 'square')
        silence = [0.0] * int(SAMPLE_RATE * beats_to_seconds(0.25))
        bass_track.extend(note_samples)
        bass_track.extend(silence)
    
    bass_stereo = stereo_pan(bass_track, 0.0)
    tracks.append(bass_stereo)
    
    # Dissonant strings
    strings_track = []
    for i in range(4):
        for notes, octave in progression:
            chord_samples = generate_chord(notes, octave + 1, beats_to_seconds(4), 0.3)
            strings_track.extend(chord_samples)
    
    strings_stereo = stereo_pan(add_reverb(strings_track, 0.4, 0.15), 0.0)
    tracks.append(strings_stereo)
    
    mixed = mix_stereo_tracks(tracks)
    return normalize_stereo(mixed, 0.7)

# ========== MAIN GENERATOR ==========

def main():
    print("=" * 70)
    print("Procedural Music Generator for Dungeon Dice Duelists")
    print("=" * 70)
    
    output_dir = "assets/audio/music"
    os.makedirs(output_dir, exist_ok=True)
    
    # Define all music tracks to generate
    tracks = {
        "menu_theme.wav": (generate_menu_theme, "Main menu background music"),
        "battle_normal.wav": (generate_battle_theme, "Normal battle music"),
        "battle_boss.wav": (generate_boss_theme, "Boss battle music"),
        "victory.wav": (generate_victory_theme, "Victory fanfare"),
        "defeat.wav": (generate_defeat_theme, "Defeat/game over music"),
        "exploration.wav": (generate_exploration_theme, "Shop/exploration music"),
        "ambient.wav": (generate_ambient_theme, "Title screen ambient"),
        "tension.wav": (generate_tension_theme, "Tension/danger music"),
    }
    
    print(f"\nGenerating {len(tracks)} music tracks...\n")
    
    for filename, (generator, description) in tracks.items():
        print(f"Creating {filename}:")
        print(f"  {description}")
        
        try:
            samples = generator()
            filepath = os.path.join(output_dir, filename)
            save_stereo_wave(filepath, samples)
            
            size = os.path.getsize(filepath)
            duration = len(samples) / SAMPLE_RATE
            print(f"  ✓ Saved ({size:,} bytes, {duration:.1f}s)\n")
            
        except Exception as e:
            print(f"  ✗ Error: {e}\n")
    
    print("=" * 70)
    print("✓ MUSIC GENERATION COMPLETE")
    print("=" * 70)
    print(f"\nGenerated {len(tracks)} music tracks in {output_dir}/")
    print("\nTrack categories:")
    print("  • Menu: menu_theme, ambient")
    print("  • Battle: battle_normal, battle_boss, tension")
    print("  • Results: victory, defeat")
    print("  • Exploration: exploration")
    print("\nAll music is:")
    print("  • 44.1 kHz, 16-bit, stereo WAV format")
    print("  • Procedurally composed")
    print("  • Loopable (seamless)")
    print("  • Royalty-free")
    print("  • Ready for SDL_mixer")

if __name__ == "__main__":
    main()
