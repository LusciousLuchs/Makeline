// Makeline - sounds, all made here from simple tones (no recordings yet).
#include "game.hpp"
#include <cmath>
#include <cstdlib>
#include <vector>

namespace {
Sound gS[SFX_N];
bool gOn = false;
float gRingT = 0;
const int RATE = 22050;

// f0 -> f1 over the length; shape 0 sine, 1 square, 2 noise
void tone(std::vector<short>& out, float secs, float f0, float f1, float vol, int shape, float decay) {
    int n = (int)(secs * RATE);
    float ph = 0;
    for (int i = 0; i < n; i++) {
        float u = (float)i / n, f = f0 + (f1 - f0) * u;
        ph += f / RATE;
        float v = shape == 0 ? std::sin(ph * 6.2832f) : shape == 1 ? (std::fmod(ph, 1.f) < 0.5f ? 0.6f : -0.6f) : (rand() / (float)RAND_MAX * 2 - 1);
        float env = std::exp(-u * decay) * std::fmin(1.f, i / 60.f) * std::fmin(1.f, (n - i) / 60.f);
        out.push_back((short)(v * env * vol * 32000));
    }
}
void gap(std::vector<short>& out, float secs) { out.insert(out.end(), (size_t)(secs * RATE), 0); }
Sound make(const std::vector<short>& s) {
    Wave w{};
    w.frameCount = (unsigned)s.size(); w.sampleRate = RATE; w.sampleSize = 16; w.channels = 1;
    w.data = (void*)s.data();
    return LoadSoundFromWave(w);   // (copies the samples)
}
}  // namespace

void audioInit() {
    InitAudioDevice();
    gOn = IsAudioDeviceReady();
    if (!gOn) return;
    std::vector<short> s;
    tone(s, 0.03f, 900, 700, 0.25f, 1, 3); gS[SFX_CLICK] = make(s); s.clear();
    for (int k = 0; k < 2; k++) { for (int i = 0; i < 9; i++) tone(s, 0.045f, i % 2 ? 1250 : 1000, i % 2 ? 1250 : 1000, 0.3f, 0, 0.5f); gap(s, 0.2f); }
    gS[SFX_RING] = make(s); s.clear();
    tone(s, 0.5f, 1320, 1320, 0.3f, 0, 5); gS[SFX_DING] = make(s); s.clear();
    tone(s, 0.03f, 240, 200, 0.2f, 1, 2); gS[SFX_TALK] = make(s); s.clear();
    tone(s, 0.4f, 220, 110, 0.3f, 1, 2); gS[SFX_BAD] = make(s); s.clear();
    tone(s, 0.09f, 880, 880, 0.28f, 0, 2); tone(s, 0.3f, 1320, 1320, 0.28f, 0, 4); gS[SFX_GOOD] = make(s); s.clear();
    tone(s, 0.12f, 660, 660, 0.25f, 1, 1); gap(s, 0.06f); tone(s, 0.12f, 660, 660, 0.25f, 1, 1); gS[SFX_OVEN] = make(s); s.clear();
    tone(s, 0.12f, 0, 0, 0.22f, 2, 4); gS[SFX_CUT] = make(s); s.clear();
    tone(s, 0.07f, 320, 140, 0.3f, 0, 3); gS[SFX_PLOP] = make(s); s.clear();
    tone(s, 0.2f, 660, 660, 0.22f, 0, 3); tone(s, 0.35f, 520, 520, 0.22f, 0, 4); gS[SFX_DOOR] = make(s); s.clear();
}
void sfx(int which) {
    if (!gOn || which < 0 || which >= SFX_N) return;
    if (which == SFX_TALK || which == SFX_PLOP) SetSoundPitch(gS[which], 0.85f + (rand() % 100) / 300.f);
    SetSoundVolume(gS[which], G.sfxVol);
    PlaySound(gS[which]);
}
void audioUpdate(bool ringing) {
    if (!gOn) return;
    gRingT -= GetFrameTime();
    if (ringing && gRingT <= 0) { SetSoundVolume(gS[SFX_RING], G.sfxVol); PlaySound(gS[SFX_RING]); gRingT = 1.9f; }
    if (!ringing) { if (IsSoundPlaying(gS[SFX_RING])) StopSound(gS[SFX_RING]); gRingT = 0; }
}
