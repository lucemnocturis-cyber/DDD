#include "MusicManager.h"
#include "../Utils/Logger.h"

#include <cmath>
#include <algorithm>
#include <cstring>

namespace DDD {

// Global instance
MusicManager* g_musicManager = nullptr;

// Musical notes (semitones from A4=440Hz)
static constexpr int NOTE_C = -9;
static constexpr int NOTE_D = -7;
static constexpr int NOTE_E = -5;
static constexpr int NOTE_F = -4;
static constexpr int NOTE_G = -2;
static constexpr int NOTE_A = 0;
static constexpr int NOTE_B = 2;

MusicManager::MusicManager() = default;

MusicManager::~MusicManager() {
    Shutdown();
}

bool MusicManager::Initialize() {
    if (m_initialized) return true;
    
    // Request audio spec
    SDL_AudioSpec desired;
    SDL_memset(&desired, 0, sizeof(desired));
    desired.freq = m_sampleRate;
    desired.format = AUDIO_S16SYS;
    desired.channels = 1;
    desired.samples = 4096;
    desired.callback = AudioCallback;
    desired.userdata = this;
    
    m_audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &m_audioSpec, 0);
    if (m_audioDevice == 0) {
        Logger::Warning("MusicManager: Failed to open audio device: {}", SDL_GetError());
        return false;
    }
    
    m_initialized = true;
    g_musicManager = this;
    
    Logger::Info("MusicManager initialized");
    return true;
}

void MusicManager::Shutdown() {
    if (m_audioDevice != 0) {
        SDL_CloseAudioDevice(m_audioDevice);
        m_audioDevice = 0;
    }
    m_trackData.clear();
    m_initialized = false;
    if (g_musicManager == this) {
        g_musicManager = nullptr;
    }
}

void MusicManager::Play(MusicTrack track, float fadeIn) {
    if (!m_initialized || track == MusicTrack::None) return;
    
    if (track == m_currentTrack && m_playing) return;
    
    // If currently playing, fade out first then switch
    if (m_playing && m_currentTrack != MusicTrack::None) {
        m_pendingTrack = track;
        m_fadeSpeed = -1.0f / std::max(0.1f, fadeIn * 0.5f);
        return;
    }
    
    // Generate new track
    GenerateTrack(track);
    m_currentTrack = track;
    m_playPosition = 0;
    m_playing = true;
    m_paused = false;
    
    // Start fade in
    m_currentVolume = 0.0f;
    m_fadeSpeed = 1.0f / std::max(0.1f, fadeIn);
    
    SDL_PauseAudioDevice(m_audioDevice, 0);
    Logger::Info("Playing music track: {}", static_cast<int>(track));
}

void MusicManager::Stop(float fadeOut) {
    if (!m_playing) return;
    
    if (fadeOut > 0) {
        m_fadeSpeed = -1.0f / fadeOut;
    } else {
        m_playing = false;
        m_currentTrack = MusicTrack::None;
        m_currentVolume = 0.0f;
        SDL_PauseAudioDevice(m_audioDevice, 1);
    }
}

void MusicManager::Pause() {
    if (m_playing && !m_paused) {
        m_paused = true;
        SDL_PauseAudioDevice(m_audioDevice, 1);
    }
}

void MusicManager::Resume() {
    if (m_playing && m_paused) {
        m_paused = false;
        SDL_PauseAudioDevice(m_audioDevice, 0);
    }
}

void MusicManager::SetVolume(float volume) {
    m_targetVolume = std::clamp(volume, 0.0f, 1.0f);
}

void MusicManager::SetMuted(bool muted) {
    m_muted = muted;
}

void MusicManager::Update(float deltaTime) {
    if (!m_initialized) return;
    
    // Handle volume fading
    if (m_fadeSpeed != 0.0f) {
        m_currentVolume += m_fadeSpeed * deltaTime;
        
        if (m_currentVolume >= m_targetVolume) {
            m_currentVolume = m_targetVolume;
            m_fadeSpeed = 0.0f;
        } else if (m_currentVolume <= 0.0f) {
            m_currentVolume = 0.0f;
            m_fadeSpeed = 0.0f;
            
            // Check if we need to switch tracks
            if (m_pendingTrack != MusicTrack::None) {
                GenerateTrack(m_pendingTrack);
                m_currentTrack = m_pendingTrack;
                m_pendingTrack = MusicTrack::None;
                m_playPosition = 0;
                m_fadeSpeed = 2.0f;  // Fade in
            } else {
                m_playing = false;
                m_currentTrack = MusicTrack::None;
                SDL_PauseAudioDevice(m_audioDevice, 1);
            }
        }
    }
}

void MusicManager::AudioCallback(void* userdata, uint8_t* stream, int len) {
    MusicManager* manager = static_cast<MusicManager*>(userdata);
    manager->MixAudio(stream, len);
}

void MusicManager::MixAudio(uint8_t* stream, int len) {
    std::memset(stream, 0, len);
    
    if (!m_playing || m_paused || m_muted || m_trackData.empty()) return;
    
    int16_t* output = reinterpret_cast<int16_t*>(stream);
    int numSamples = len / sizeof(int16_t);
    
    float volume = m_currentVolume * m_targetVolume;
    
    for (int i = 0; i < numSamples; ++i) {
        if (m_playPosition >= m_trackData.size()) {
            m_playPosition = 0;  // Loop
        }
        
        int32_t sample = static_cast<int32_t>(m_trackData[m_playPosition] * volume);
        output[i] = static_cast<int16_t>(std::clamp(sample, -32768, 32767));
        ++m_playPosition;
    }
}

void MusicManager::GenerateTrack(MusicTrack track) {
    switch (track) {
        case MusicTrack::Menu:      GenerateMenuMusic(); break;
        case MusicTrack::Battle:    GenerateBattleMusic(); break;
        case MusicTrack::BossBattle: GenerateBossMusic(); break;
        case MusicTrack::Victory:   GenerateVictoryMusic(); break;
        case MusicTrack::Defeat:    GenerateDefeatMusic(); break;
        case MusicTrack::Shop:      GenerateShopMusic(); break;
        default: break;
    }
}

float MusicManager::NoteToFreq(int note, int octave) {
    // A4 = 440Hz, each semitone is 2^(1/12) ratio
    float a4 = 440.0f;
    int semitonesFromA4 = note + (octave - 4) * 12;
    return a4 * std::pow(2.0f, semitonesFromA4 / 12.0f);
}

std::vector<int> MusicManager::GetScale(MusicalMode mode) {
    switch (mode) {
        case MusicalMode::Major:
            return {0, 2, 4, 5, 7, 9, 11};  // W W H W W W H
        case MusicalMode::Minor:
            return {0, 2, 3, 5, 7, 8, 10};  // W H W W H W W
        case MusicalMode::Dorian:
            return {0, 2, 3, 5, 7, 9, 10};  // W H W W W H W
        case MusicalMode::Pentatonic:
            return {0, 2, 4, 7, 9};         // Major pentatonic
        default:
            return {0, 2, 4, 5, 7, 9, 11};
    }
}

void MusicManager::AddNote(float frequency, float startTime, float duration, 
                           float volume, int waveform) {
    int startSample = static_cast<int>(startTime * m_sampleRate);
    int numSamples = static_cast<int>(duration * m_sampleRate);
    
    // Ensure buffer is large enough
    size_t endSample = startSample + numSamples;
    if (endSample > m_mixBuffer.size()) {
        m_mixBuffer.resize(endSample, 0.0f);
    }
    
    float phase = 0.0f;
    float phaseInc = frequency / m_sampleRate;
    
    for (int i = 0; i < numSamples; ++i) {
        float t = static_cast<float>(i) / numSamples;
        
        // ADSR envelope
        float env = 1.0f;
        if (t < 0.05f) {
            env = t / 0.05f;  // Attack
        } else if (t < 0.15f) {
            env = 1.0f - (t - 0.05f) / 0.1f * 0.3f;  // Decay
        } else if (t < 0.8f) {
            env = 0.7f;  // Sustain
        } else {
            env = 0.7f * (1.0f - (t - 0.8f) / 0.2f);  // Release
        }
        
        float sample = 0.0f;
        switch (waveform) {
            case 0: // Square
                sample = phase < 0.5f ? 1.0f : -1.0f;
                break;
            case 1: // Sine
                sample = std::sin(2.0f * 3.14159f * phase);
                break;
            case 2: // Triangle
                sample = 4.0f * std::abs(phase - 0.5f) - 1.0f;
                break;
            case 3: // Sawtooth
                sample = 2.0f * phase - 1.0f;
                break;
        }
        
        m_mixBuffer[startSample + i] += sample * env * volume;
        
        phase += phaseInc;
        if (phase >= 1.0f) phase -= 1.0f;
    }
}

void MusicManager::AddChord(const std::vector<float>& frequencies, float startTime, 
                            float duration, float volume) {
    float noteVol = volume / std::sqrt(static_cast<float>(frequencies.size()));
    for (float freq : frequencies) {
        AddNote(freq, startTime, duration, noteVol, 1);  // Sine waves for chords
    }
}

void MusicManager::AddDrum(float startTime, int type, float volume) {
    int startSample = static_cast<int>(startTime * m_sampleRate);
    int numSamples = static_cast<int>(0.1f * m_sampleRate);  // 100ms
    
    size_t endSample = startSample + numSamples;
    if (endSample > m_mixBuffer.size()) {
        m_mixBuffer.resize(endSample, 0.0f);
    }
    
    for (int i = 0; i < numSamples; ++i) {
        float t = static_cast<float>(i) / numSamples;
        float env = std::exp(-t * 20.0f);  // Quick decay
        
        float sample = 0.0f;
        if (type == 0) {
            // Kick drum - low frequency sine with pitch drop
            float freq = 150.0f * (1.0f - t * 0.5f);
            sample = std::sin(2.0f * 3.14159f * freq * t) * env;
        } else {
            // Hi-hat - noise
            sample = ((static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f) * env * 0.5f;
        }
        
        m_mixBuffer[startSample + i] += sample * volume;
    }
}

void MusicManager::AddBass(float frequency, float startTime, float duration, float volume) {
    int startSample = static_cast<int>(startTime * m_sampleRate);
    int numSamples = static_cast<int>(duration * m_sampleRate);
    
    size_t endSample = startSample + numSamples;
    if (endSample > m_mixBuffer.size()) {
        m_mixBuffer.resize(endSample, 0.0f);
    }
    
    float phase = 0.0f;
    float phaseInc = frequency / m_sampleRate;
    
    for (int i = 0; i < numSamples; ++i) {
        float t = static_cast<float>(i) / numSamples;
        float env = std::min(t * 10.0f, 1.0f) * (1.0f - t);  // Quick attack, gradual release
        
        // Square wave for punchy bass
        float sample = phase < 0.5f ? 1.0f : -1.0f;
        
        m_mixBuffer[startSample + i] += sample * env * volume;
        
        phase += phaseInc;
        if (phase >= 1.0f) phase -= 1.0f;
    }
}

void MusicManager::GenerateMenuMusic() {
    m_trackLength = 16.0f;  // 16 second loop
    m_mixBuffer.clear();
    m_mixBuffer.resize(static_cast<size_t>(m_trackLength * m_sampleRate), 0.0f);
    
    // Calm, ambient music in C major
    // Pad chords
    std::vector<std::vector<float>> chords = {
        {NoteToFreq(NOTE_C, 3), NoteToFreq(NOTE_E, 3), NoteToFreq(NOTE_G, 3)},  // C
        {NoteToFreq(NOTE_A, 2), NoteToFreq(NOTE_C, 3), NoteToFreq(NOTE_E, 3)},  // Am
        {NoteToFreq(NOTE_F, 2), NoteToFreq(NOTE_A, 2), NoteToFreq(NOTE_C, 3)},  // F
        {NoteToFreq(NOTE_G, 2), NoteToFreq(NOTE_B, 2), NoteToFreq(NOTE_D, 3)}   // G
    };
    
    for (int bar = 0; bar < 4; ++bar) {
        float barStart = bar * 4.0f;
        AddChord(chords[bar], barStart, 4.0f, 0.12f);
        
        // Gentle melody
        float melody[] = {NOTE_E, NOTE_G, NOTE_A, NOTE_G};
        for (int note = 0; note < 4; ++note) {
            AddNote(NoteToFreq(melody[note], 4), barStart + note, 0.8f, 0.15f, 1);
        }
    }
    
    // Convert to int16
    m_trackData.resize(m_mixBuffer.size());
    for (size_t i = 0; i < m_mixBuffer.size(); ++i) {
        float clamped = std::clamp(m_mixBuffer[i], -1.0f, 1.0f);
        m_trackData[i] = static_cast<int16_t>(clamped * 24000);
    }
    
    Logger::Debug("Generated menu music: {} samples", m_trackData.size());
}

void MusicManager::GenerateBattleMusic() {
    m_trackLength = 8.0f;  // 8 second loop, 120 BPM
    float bpm = 120.0f;
    float beatLen = 60.0f / bpm;
    
    m_mixBuffer.clear();
    m_mixBuffer.resize(static_cast<size_t>(m_trackLength * m_sampleRate), 0.0f);
    
    // D minor - energetic
    for (int bar = 0; bar < 4; ++bar) {
        float barStart = bar * 2.0f;
        
        // Driving drums
        for (int beat = 0; beat < 4; ++beat) {
            AddDrum(barStart + beat * beatLen, 0, 0.35f);  // Kick on each beat
            AddDrum(barStart + beat * beatLen + beatLen * 0.5f, 1, 0.2f);  // Hi-hat off-beat
        }
        
        // Bass line (D minor)
        float bassNotes[] = {NOTE_D, NOTE_D, NOTE_F, NOTE_A - 12};
        for (int i = 0; i < 4; ++i) {
            AddBass(NoteToFreq(bassNotes[i], 2), barStart + i * beatLen, beatLen * 0.8f, 0.25f);
        }
        
        // Power chord stabs
        if (bar % 2 == 0) {
            AddChord({NoteToFreq(NOTE_D, 3), NoteToFreq(NOTE_A, 3)}, barStart, beatLen * 2, 0.2f);
        } else {
            AddChord({NoteToFreq(NOTE_F, 3), NoteToFreq(NOTE_C, 4)}, barStart, beatLen * 2, 0.2f);
        }
    }
    
    // Convert
    m_trackData.resize(m_mixBuffer.size());
    for (size_t i = 0; i < m_mixBuffer.size(); ++i) {
        float clamped = std::clamp(m_mixBuffer[i], -1.0f, 1.0f);
        m_trackData[i] = static_cast<int16_t>(clamped * 24000);
    }
    
    Logger::Debug("Generated battle music: {} samples", m_trackData.size());
}

void MusicManager::GenerateBossMusic() {
    m_trackLength = 8.0f;
    float bpm = 140.0f;  // Faster
    float beatLen = 60.0f / bpm;
    
    m_mixBuffer.clear();
    m_mixBuffer.resize(static_cast<size_t>(m_trackLength * m_sampleRate), 0.0f);
    
    // E minor - dark and intense
    for (int bar = 0; bar < 4; ++bar) {
        float barStart = bar * 2.0f;
        
        // Double-time drums
        for (int beat = 0; beat < 8; ++beat) {
            AddDrum(barStart + beat * beatLen * 0.5f, 0, 0.3f);
            if (beat % 2 == 1) {
                AddDrum(barStart + beat * beatLen * 0.5f, 1, 0.25f);
            }
        }
        
        // Aggressive bass
        AddBass(NoteToFreq(NOTE_E, 2), barStart, beatLen * 2, 0.35f);
        AddBass(NoteToFreq(NOTE_G, 2), barStart + beatLen * 2, beatLen, 0.35f);
        AddBass(NoteToFreq(NOTE_A, 2), barStart + beatLen * 3, beatLen, 0.35f);
        
        // Dissonant stabs
        AddChord({NoteToFreq(NOTE_E, 3), NoteToFreq(NOTE_B, 3), NoteToFreq(NOTE_D, 4)}, 
                 barStart + beatLen, beatLen * 0.5f, 0.25f);
    }
    
    // Convert
    m_trackData.resize(m_mixBuffer.size());
    for (size_t i = 0; i < m_mixBuffer.size(); ++i) {
        float clamped = std::clamp(m_mixBuffer[i], -1.0f, 1.0f);
        m_trackData[i] = static_cast<int16_t>(clamped * 24000);
    }
}

void MusicManager::GenerateVictoryMusic() {
    m_trackLength = 4.0f;  // Short fanfare
    
    m_mixBuffer.clear();
    m_mixBuffer.resize(static_cast<size_t>(m_trackLength * m_sampleRate), 0.0f);
    
    // C major fanfare
    AddNote(NoteToFreq(NOTE_C, 4), 0.0f, 0.3f, 0.3f, 0);
    AddNote(NoteToFreq(NOTE_E, 4), 0.3f, 0.3f, 0.3f, 0);
    AddNote(NoteToFreq(NOTE_G, 4), 0.6f, 0.3f, 0.3f, 0);
    AddNote(NoteToFreq(NOTE_C, 5), 0.9f, 1.0f, 0.35f, 0);
    
    AddChord({NoteToFreq(NOTE_C, 3), NoteToFreq(NOTE_E, 3), NoteToFreq(NOTE_G, 3), NoteToFreq(NOTE_C, 4)},
             1.2f, 2.5f, 0.25f);
    
    // Convert
    m_trackData.resize(m_mixBuffer.size());
    for (size_t i = 0; i < m_mixBuffer.size(); ++i) {
        float clamped = std::clamp(m_mixBuffer[i], -1.0f, 1.0f);
        m_trackData[i] = static_cast<int16_t>(clamped * 24000);
    }
}

void MusicManager::GenerateDefeatMusic() {
    m_trackLength = 4.0f;
    
    m_mixBuffer.clear();
    m_mixBuffer.resize(static_cast<size_t>(m_trackLength * m_sampleRate), 0.0f);
    
    // Descending minor - sad
    AddNote(NoteToFreq(NOTE_A, 4), 0.0f, 0.8f, 0.25f, 1);
    AddNote(NoteToFreq(NOTE_G, 4), 0.6f, 0.8f, 0.25f, 1);
    AddNote(NoteToFreq(NOTE_F, 4), 1.2f, 0.8f, 0.25f, 1);
    AddNote(NoteToFreq(NOTE_E, 4), 1.8f, 1.5f, 0.25f, 1);
    
    AddChord({NoteToFreq(NOTE_A, 2), NoteToFreq(NOTE_C, 3), NoteToFreq(NOTE_E, 3)},
             2.5f, 1.5f, 0.2f);
    
    // Convert
    m_trackData.resize(m_mixBuffer.size());
    for (size_t i = 0; i < m_mixBuffer.size(); ++i) {
        float clamped = std::clamp(m_mixBuffer[i], -1.0f, 1.0f);
        m_trackData[i] = static_cast<int16_t>(clamped * 24000);
    }
}

void MusicManager::GenerateShopMusic() {
    m_trackLength = 12.0f;
    float bpm = 90.0f;
    float beatLen = 60.0f / bpm;
    
    m_mixBuffer.clear();
    m_mixBuffer.resize(static_cast<size_t>(m_trackLength * m_sampleRate), 0.0f);
    
    // G major - pleasant, relaxed
    std::vector<int> melody = {NOTE_G, NOTE_A, NOTE_B, NOTE_D + 12, NOTE_B, NOTE_A, NOTE_G, NOTE_E};
    
    for (int bar = 0; bar < 4; ++bar) {
        float barStart = bar * 3.0f;
        
        // Soft pad
        AddChord({NoteToFreq(NOTE_G, 3), NoteToFreq(NOTE_B, 3), NoteToFreq(NOTE_D, 4)},
                 barStart, 3.0f, 0.1f);
        
        // Melody
        for (int note = 0; note < 6; ++note) {
            int idx = (bar * 2 + note) % melody.size();
            AddNote(NoteToFreq(melody[idx], 4), barStart + note * beatLen, beatLen * 0.8f, 0.18f, 1);
        }
    }
    
    // Convert
    m_trackData.resize(m_mixBuffer.size());
    for (size_t i = 0; i < m_mixBuffer.size(); ++i) {
        float clamped = std::clamp(m_mixBuffer[i], -1.0f, 1.0f);
        m_trackData[i] = static_cast<int16_t>(clamped * 24000);
    }
}

} // namespace DDD
