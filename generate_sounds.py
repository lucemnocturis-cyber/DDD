#!/usr/bin/env python3
"""
Procedural Sound Effect Generator for Dungeon Dice Duelists
Generates game sound effects using wave synthesis (no external audio libraries needed)
"""

import wave
import struct
import math
import random
import os

# Sound configuration
SAMPLE_RATE = 44100  # CD quality
CHANNELS = 1  # Mono
SAMPLE_WIDTH = 2  # 16-bit

def generate_sine_wave(frequency, duration, amplitude=0.5):
    """Generate a sine wave"""
    samples = []
    num_samples = int(SAMPLE_RATE * duration)
    
    for i in range(num_samples):
        t = i / SAMPLE_RATE
        value = amplitude * math.sin(2 * math.pi * frequency * t)
        samples.append(value)
    
    return samples

def generate_square_wave(frequency, duration, amplitude=0.5):
    """Generate a square wave"""
    samples = []
    num_samples = int(SAMPLE_RATE * duration)
    
    for i in range(num_samples):
        t = i / SAMPLE_RATE
        value = amplitude if math.sin(2 * math.pi * frequency * t) > 0 else -amplitude
        samples.append(value)
    
    return samples

def generate_sawtooth_wave(frequency, duration, amplitude=0.5):
    """Generate a sawtooth wave"""
    samples = []
    num_samples = int(SAMPLE_RATE * duration)
    
    for i in range(num_samples):
        t = i / SAMPLE_RATE
        phase = (frequency * t) % 1.0
        value = amplitude * (2 * phase - 1)
        samples.append(value)
    
    return samples

def generate_noise(duration, amplitude=0.5):
    """Generate white noise"""
    samples = []
    num_samples = int(SAMPLE_RATE * duration)
    
    for i in range(num_samples):
        value = amplitude * (2 * random.random() - 1)
        samples.append(value)
    
    return samples

def apply_envelope(samples, attack=0.01, decay=0.1, sustain=0.7, release=0.2):
    """Apply ADSR envelope to samples"""
    num_samples = len(samples)
    attack_samples = int(SAMPLE_RATE * attack)
    decay_samples = int(SAMPLE_RATE * decay)
    release_samples = int(SAMPLE_RATE * release)
    sustain_samples = num_samples - attack_samples - decay_samples - release_samples
    
    enveloped = []
    
    for i, sample in enumerate(samples):
        if i < attack_samples:
            # Attack: 0 to 1
            envelope = i / attack_samples
        elif i < attack_samples + decay_samples:
            # Decay: 1 to sustain level
            progress = (i - attack_samples) / decay_samples
            envelope = 1.0 - (1.0 - sustain) * progress
        elif i < attack_samples + decay_samples + sustain_samples:
            # Sustain: constant
            envelope = sustain
        else:
            # Release: sustain to 0
            progress = (i - attack_samples - decay_samples - sustain_samples) / release_samples
            envelope = sustain * (1.0 - progress)
        
        enveloped.append(sample * envelope)
    
    return enveloped

def mix_samples(samples_list, weights=None):
    """Mix multiple sample lists together"""
    if not samples_list:
        return []
    
    if weights is None:
        weights = [1.0] * len(samples_list)
    
    # Normalize weights
    total_weight = sum(weights)
    weights = [w / total_weight for w in weights]
    
    # Find max length
    max_len = max(len(s) for s in samples_list)
    
    # Mix
    mixed = []
    for i in range(max_len):
        value = 0
        for samples, weight in zip(samples_list, weights):
            if i < len(samples):
                value += samples[i] * weight
        mixed.append(value)
    
    return mixed

def normalize_samples(samples, target_amplitude=0.8):
    """Normalize samples to target amplitude"""
    if not samples:
        return samples
    
    max_val = max(abs(s) for s in samples)
    if max_val > 0:
        scale = target_amplitude / max_val
        return [s * scale for s in samples]
    return samples

def samples_to_bytes(samples):
    """Convert float samples to 16-bit PCM bytes"""
    data = []
    for sample in samples:
        # Clamp to [-1, 1]
        sample = max(-1.0, min(1.0, sample))
        # Convert to 16-bit signed integer
        value = int(sample * 32767)
        data.append(struct.pack('<h', value))
    return b''.join(data)

def save_wave_file(filename, samples):
    """Save samples as WAV file"""
    with wave.open(filename, 'w') as wav_file:
        wav_file.setnchannels(CHANNELS)
        wav_file.setsampwidth(SAMPLE_WIDTH)
        wav_file.setframerate(SAMPLE_RATE)
        wav_file.writeframes(samples_to_bytes(samples))

# ========== SOUND EFFECT GENERATORS ==========

def generate_hit_sound():
    """Generate hit/impact sound"""
    # Low thud
    low_freq = generate_sine_wave(120, 0.15, 0.8)
    # Mid punch
    mid_freq = generate_square_wave(240, 0.1, 0.5)
    # High click
    high_freq = generate_sine_wave(800, 0.05, 0.3)
    
    mixed = mix_samples([low_freq, mid_freq, high_freq], [0.5, 0.3, 0.2])
    enveloped = apply_envelope(mixed, 0.001, 0.05, 0.3, 0.1)
    
    return normalize_samples(enveloped)

def generate_sword_slash():
    """Generate sword slash/swing sound"""
    # Whoosh - descending frequency
    samples = []
    duration = 0.3
    num_samples = int(SAMPLE_RATE * duration)
    
    start_freq = 1200
    end_freq = 400
    
    for i in range(num_samples):
        t = i / SAMPLE_RATE
        progress = t / duration
        freq = start_freq + (end_freq - start_freq) * progress
        
        # Mix noise and tone
        tone = 0.4 * math.sin(2 * math.pi * freq * t)
        noise = 0.3 * (2 * random.random() - 1)
        
        samples.append(tone + noise)
    
    enveloped = apply_envelope(samples, 0.01, 0.1, 0.6, 0.2)
    return normalize_samples(enveloped)

def generate_magic_cast():
    """Generate magic spell cast sound"""
    # Rising magical chime
    samples = []
    duration = 0.6
    num_samples = int(SAMPLE_RATE * duration)
    
    for i in range(num_samples):
        t = i / SAMPLE_RATE
        progress = t / duration
        
        # Multiple harmonics
        freq1 = 440 * (1 + progress * 2)  # Rising
        freq2 = 880 * (1 + progress * 1.5)
        freq3 = 1320 * (1 + progress)
        
        tone = 0.3 * math.sin(2 * math.pi * freq1 * t)
        tone += 0.2 * math.sin(2 * math.pi * freq2 * t)
        tone += 0.1 * math.sin(2 * math.pi * freq3 * t)
        
        # Add shimmer
        shimmer = 0.1 * math.sin(2 * math.pi * 10 * t)
        
        samples.append(tone + shimmer)
    
    enveloped = apply_envelope(samples, 0.05, 0.2, 0.5, 0.3)
    return normalize_samples(enveloped)

def generate_heal_sound():
    """Generate healing/restoration sound"""
    # Gentle ascending chimes
    samples = []
    duration = 0.8
    
    # Three ascending notes
    notes = [523, 659, 784]  # C, E, G (major chord)
    
    for note_freq in notes:
        note = generate_sine_wave(note_freq, 0.4, 0.6)
        samples = mix_samples([samples, note]) if samples else note
    
    enveloped = apply_envelope(samples, 0.1, 0.2, 0.6, 0.4)
    return normalize_samples(enveloped, 0.6)

def generate_explosion():
    """Generate explosion sound"""
    # Deep boom with noise
    boom = generate_sine_wave(60, 0.4, 0.9)
    crack = generate_noise(0.15, 0.7)
    rumble = generate_sawtooth_wave(80, 0.5, 0.5)
    
    mixed = mix_samples([boom, crack, rumble], [0.5, 0.3, 0.2])
    enveloped = apply_envelope(mixed, 0.001, 0.1, 0.4, 0.4)
    
    return normalize_samples(enveloped)

def generate_dice_roll():
    """Generate dice rolling sound"""
    # Rattling/tumbling sound
    samples = []
    duration = 0.8
    num_samples = int(SAMPLE_RATE * duration)
    
    for i in range(num_samples):
        t = i / SAMPLE_RATE
        
        # Random impacts decreasing in frequency
        progress = t / duration
        impact_freq = 30 * (1 - progress * 0.7)
        
        if random.random() < impact_freq / SAMPLE_RATE:
            # Random click
            click_freq = random.randint(400, 800)
            for j in range(int(SAMPLE_RATE * 0.02)):
                if i + j < num_samples:
                    samples.append(0.5 * math.sin(2 * math.pi * click_freq * j / SAMPLE_RATE))
        else:
            samples.append(0)
    
    enveloped = apply_envelope(samples, 0.01, 0.2, 0.6, 0.2)
    return normalize_samples(enveloped, 0.5)

def generate_button_click():
    """Generate UI button click"""
    # Short click
    click = generate_sine_wave(1200, 0.05, 0.6)
    enveloped = apply_envelope(click, 0.001, 0.02, 0.3, 0.03)
    return normalize_samples(enveloped, 0.4)

def generate_button_hover():
    """Generate UI button hover"""
    # Soft beep
    beep = generate_sine_wave(800, 0.08, 0.4)
    enveloped = apply_envelope(beep, 0.005, 0.03, 0.4, 0.04)
    return normalize_samples(enveloped, 0.3)

def generate_menu_open():
    """Generate menu open sound"""
    # Rising chime
    samples = []
    duration = 0.2
    num_samples = int(SAMPLE_RATE * duration)
    
    start_freq = 400
    end_freq = 800
    
    for i in range(num_samples):
        t = i / SAMPLE_RATE
        progress = t / duration
        freq = start_freq + (end_freq - start_freq) * progress
        samples.append(0.5 * math.sin(2 * math.pi * freq * t))
    
    enveloped = apply_envelope(samples, 0.01, 0.05, 0.5, 0.1)
    return normalize_samples(enveloped, 0.5)

def generate_menu_close():
    """Generate menu close sound"""
    # Falling chime
    samples = []
    duration = 0.2
    num_samples = int(SAMPLE_RATE * duration)
    
    start_freq = 800
    end_freq = 400
    
    for i in range(num_samples):
        t = i / SAMPLE_RATE
        progress = t / duration
        freq = start_freq + (end_freq - start_freq) * progress
        samples.append(0.5 * math.sin(2 * math.pi * freq * t))
    
    enveloped = apply_envelope(samples, 0.01, 0.05, 0.5, 0.1)
    return normalize_samples(enveloped, 0.5)

def generate_error_sound():
    """Generate error/invalid action sound"""
    # Harsh buzzer
    buzz = generate_square_wave(200, 0.15, 0.7)
    enveloped = apply_envelope(buzz, 0.01, 0.05, 0.6, 0.05)
    return normalize_samples(enveloped, 0.5)

def generate_level_up():
    """Generate level up/achievement sound"""
    # Triumphant fanfare
    samples = []
    
    # Ascending arpeggio: C-E-G-C
    notes = [262, 330, 392, 524]
    
    for note_freq in notes:
        note = generate_sine_wave(note_freq, 0.2, 0.7)
        samples = mix_samples([samples, note]) if samples else note
    
    enveloped = apply_envelope(samples, 0.05, 0.1, 0.7, 0.3)
    return normalize_samples(enveloped, 0.7)

def generate_death_sound():
    """Generate unit death sound"""
    # Descending sad sound
    samples = []
    duration = 0.6
    num_samples = int(SAMPLE_RATE * duration)
    
    start_freq = 400
    end_freq = 100
    
    for i in range(num_samples):
        t = i / SAMPLE_RATE
        progress = t / duration
        freq = start_freq + (end_freq - start_freq) * progress
        samples.append(0.6 * math.sin(2 * math.pi * freq * t))
    
    enveloped = apply_envelope(samples, 0.05, 0.2, 0.4, 0.3)
    return normalize_samples(enveloped, 0.6)

def generate_victory_sound():
    """Generate victory fanfare"""
    # Major chord progression
    samples = []
    
    # Victory chord: C major
    chord = [
        generate_sine_wave(262, 1.5, 0.5),  # C
        generate_sine_wave(330, 1.5, 0.4),  # E
        generate_sine_wave(392, 1.5, 0.3),  # G
    ]
    
    samples = mix_samples(chord)
    enveloped = apply_envelope(samples, 0.1, 0.3, 0.7, 0.5)
    return normalize_samples(enveloped, 0.8)

def generate_footstep():
    """Generate footstep sound"""
    # Thud
    thud = generate_sine_wave(100, 0.1, 0.6)
    scuff = generate_noise(0.08, 0.2)
    
    mixed = mix_samples([thud, scuff], [0.7, 0.3])
    enveloped = apply_envelope(mixed, 0.001, 0.03, 0.3, 0.05)
    return normalize_samples(enveloped, 0.4)

def generate_arrow_shoot():
    """Generate arrow shooting sound"""
    # Twang and whoosh
    twang = generate_sawtooth_wave(300, 0.1, 0.7)
    whoosh = generate_noise(0.3, 0.3)
    
    mixed = mix_samples([twang, whoosh], [0.6, 0.4])
    enveloped = apply_envelope(mixed, 0.001, 0.05, 0.5, 0.2)
    return normalize_samples(enveloped, 0.6)

def generate_shield_block():
    """Generate shield block sound"""
    # Metallic clang
    clang1 = generate_square_wave(600, 0.15, 0.7)
    clang2 = generate_square_wave(900, 0.12, 0.5)
    
    mixed = mix_samples([clang1, clang2], [0.6, 0.4])
    enveloped = apply_envelope(mixed, 0.001, 0.05, 0.3, 0.1)
    return normalize_samples(enveloped, 0.7)

def generate_coin_collect():
    """Generate coin collection sound"""
    # Cheerful ding
    ding = generate_sine_wave(1200, 0.15, 0.6)
    ding2 = generate_sine_wave(1600, 0.12, 0.4)
    
    mixed = mix_samples([ding, ding2])
    enveloped = apply_envelope(mixed, 0.01, 0.05, 0.5, 0.08)
    return normalize_samples(enveloped, 0.6)

def generate_power_up():
    """Generate power-up sound"""
    # Ascending sparkle
    samples = []
    duration = 0.5
    num_samples = int(SAMPLE_RATE * duration)
    
    for i in range(num_samples):
        t = i / SAMPLE_RATE
        progress = t / duration
        
        freq = 440 * (1 + progress * 3)
        tone = 0.5 * math.sin(2 * math.pi * freq * t)
        
        # Add sparkle
        sparkle = 0.2 * math.sin(2 * math.pi * freq * 3 * t)
        
        samples.append(tone + sparkle)
    
    enveloped = apply_envelope(samples, 0.01, 0.1, 0.6, 0.2)
    return normalize_samples(enveloped, 0.7)

# ========== MAIN GENERATOR ==========

def main():
    print("=" * 70)
    print("Procedural Sound Effect Generator for Dungeon Dice Duelists")
    print("=" * 70)
    
    output_dir = "assets/audio/sfx"
    os.makedirs(output_dir, exist_ok=True)
    
    # Define all sounds to generate
    sounds = {
        # Combat sounds
        "hit.wav": generate_hit_sound,
        "sword_slash.wav": generate_sword_slash,
        "magic_cast.wav": generate_magic_cast,
        "explosion.wav": generate_explosion,
        "arrow_shoot.wav": generate_arrow_shoot,
        "shield_block.wav": generate_shield_block,
        
        # Character sounds
        "heal.wav": generate_heal_sound,
        "death.wav": generate_death_sound,
        "level_up.wav": generate_level_up,
        "footstep.wav": generate_footstep,
        "power_up.wav": generate_power_up,
        
        # UI sounds
        "button_click.wav": generate_button_click,
        "button_hover.wav": generate_button_hover,
        "menu_open.wav": generate_menu_open,
        "menu_close.wav": generate_menu_close,
        "error.wav": generate_error_sound,
        "coin_collect.wav": generate_coin_collect,
        
        # Game sounds
        "dice_roll.wav": generate_dice_roll,
        "victory.wav": generate_victory_sound,
    }
    
    print(f"\nGenerating {len(sounds)} sound effects...\n")
    
    for filename, generator in sounds.items():
        print(f"Generating {filename}...", end=" ")
        try:
            samples = generator()
            filepath = os.path.join(output_dir, filename)
            save_wave_file(filepath, samples)
            
            # Get file size
            size = os.path.getsize(filepath)
            print(f"✓ ({size:,} bytes)")
        except Exception as e:
            print(f"✗ Error: {e}")
    
    print("\n" + "=" * 70)
    print("✓ SOUND GENERATION COMPLETE")
    print("=" * 70)
    print(f"\nGenerated {len(sounds)} sound effects in {output_dir}/")
    print("\nSound categories:")
    print("  • Combat: hit, slash, magic, explosion, arrow, shield")
    print("  • Character: heal, death, level up, footstep, power up")
    print("  • UI: click, hover, menu, error, coin")
    print("  • Game: dice roll, victory")
    print("\nAll sounds are:")
    print("  • 44.1 kHz, 16-bit, mono WAV format")
    print("  • Procedurally generated (no samples)")
    print("  • Royalty-free (generated code)")
    print("  • Ready for SDL_mixer")

if __name__ == "__main__":
    main()
