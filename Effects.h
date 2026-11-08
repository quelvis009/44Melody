#pragma once
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <utility>

namespace mfx
{
constexpr float kTwoPi = 6.28318530718f;

struct Rng
{
    uint32_t s = 22222u;
    inline float next()
    {
        s = s * 1664525u + 1013904223u;
        return (float) (s >> 8) * (1.0f / 16777216.0f);
    }
    inline float range (float a, float b) { return a + (b - a) * next(); }
};

struct DelayBuf
{
    std::vector<float> buf;
    int size = 8;
    int w = 0;

    void prepare (int maxSamples)
    {
        size = std::max (8, maxSamples + 4);
        buf.assign ((size_t) size, 0.0f);
        w = 0;
    }

    inline void push (float x)
    {
        buf[(size_t) w] = x;
        if (++w >= size) w = 0;
    }

    inline float read (float d) const
    {
        d = std::min (std::max (d, 1.0f), (float) (size - 3));
        float rp = (float) w - d;
        if (rp < 0.0f) rp += (float) size;
        int i0 = (int) rp;
        if (i0 >= size) i0 = 0;
        const float f = rp - (float) i0;
        int i1 = i0 + 1;
        if (i1 >= size) i1 = 0;
        return buf[(size_t) i0] * (1.0f - f) + buf[(size_t) i1] * f;
    }
};

struct Flanger
{
    DelayBuf d[2];
    float ph = 0.0f;
    float fbs[2] = { 0.0f, 0.0f };
    double sr = 44100.0;

    void prepare (double s)
    {
        sr = s;
        for (auto& x : d) x.prepare ((int) (s * 0.012));
        ph = 0.0f; fbs[0] = fbs[1] = 0.0f;
    }

    void process (float* c0, float* c1, int n, float a)
    {
        float* c[2] = { c0, c1 };
        const float inc = kTwoPi * (0.1f + 0.5f * a) / (float) sr;
        const float fb = 0.1f + 0.55f * a;
        for (int i = 0; i < n; ++i)
        {
            ph += inc;
            if (ph > kTwoPi) ph -= kTwoPi;
            for (int ch = 0; ch < 2; ++ch)
            {
                const float lfo = std::sin (ph + (ch == 0 ? 0.0f : 1.57f));
                const float dsmp = (2.5f + 2.0f * lfo) * 0.001f * (float) sr;
                const float wet = d[ch].read (dsmp);
                const float x = c[ch][i];
                d[ch].push (x + wet * fb);
                c[ch][i] = x + wet * (0.5f + 0.4f * a);
            }
        }
    }
};

struct Vibrato
{
    DelayBuf d[2];
    float ph = 0.0f;
    double sr = 44100.0;

    void prepare (double s)
    {
        sr = s;
        for (auto& x : d) x.prepare ((int) (s * 0.03));
        ph = 0.0f;
    }

    void process (float* c0, float* c1, int n, float a)
    {
        float* c[2] = { c0, c1 };
        const float inc = kTwoPi * 5.0f / (float) sr;
        const float depthMs = 0.5f + 3.0f * a;
        const float centreMs = depthMs + 1.0f;
        const float wetAmt = std::min (1.0f, a * 2.0f);
        for (int i = 0; i < n; ++i)
        {
            ph += inc;
            if (ph > kTwoPi) ph -= kTwoPi;
            const float dsmp = (centreMs + depthMs * std::sin (ph)) * 0.001f * (float) sr;
            for (int ch = 0; ch < 2; ++ch)
            {
                const float x = c[ch][i];
                const float wet = d[ch].read (dsmp);
                d[ch].push (x);
                c[ch][i] = x * (1.0f - wetAmt) + wet * wetAmt;
            }
        }
    }
};

struct WowFlutter
{
    DelayBuf d[2];
    float p1 = 0.0f, p2 = 0.0f, p3 = 0.0f;
    double sr = 44100.0;

    void prepare (double s)
    {
        sr = s;
        for (auto& x : d) x.prepare ((int) (s * 0.05));
        p1 = p2 = p3 = 0.0f;
    }

    void process (float* c0, float* c1, int n, float a)
    {
        float* c[2] = { c0, c1 };
        const float i1 = kTwoPi * 0.6f / (float) sr;
        const float i2 = kTwoPi * 8.0f / (float) sr;
        const float i3 = kTwoPi * 0.23f / (float) sr;
        for (int i = 0; i < n; ++i)
        {
            p1 += i1; if (p1 > kTwoPi) p1 -= kTwoPi;
            p2 += i2; if (p2 > kTwoPi) p2 -= kTwoPi;
            p3 += i3; if (p3 > kTwoPi) p3 -= kTwoPi;
            const float ms = 12.0f + a * (3.0f * std::sin (p1) + 0.4f * std::sin (p2) + 1.5f * std::sin (p3));
            const float dsmp = ms * 0.001f * (float) sr;
            for (int ch = 0; ch < 2; ++ch)
            {
                const float x = c[ch][i];
                const float wet = d[ch].read (dsmp);
                d[ch].push (x);
                c[ch][i] = wet;
            }
        }
    }
};

struct PitchShifter
{
    DelayBuf d[2];
    float phase = 0.0f;
    float winSamples = 2000.0f;

    void prepare (double sr)
    {
        winSamples = (float) (0.05 * sr);
        for (auto& x : d) x.prepare ((int) winSamples + 8);
        phase = 0.0f;
    }

    inline void processSample (float x0, float x1, float ratio, float& y0, float& y1)
    {
        d[0].push (x0);
        d[1].push (x1);

        phase += (1.0f - ratio) / winSamples;
        if (phase >= 1.0f) phase -= 1.0f;
        else if (phase < 0.0f) phase += 1.0f;

        float p2 = phase + 0.5f;
        if (p2 >= 1.0f) p2 -= 1.0f;

        const float g1 = 0.5f - 0.5f * std::cos (kTwoPi * phase);
        const float g2 = 0.5f - 0.5f * std::cos (kTwoPi * p2);
        const float dl1 = 1.0f + phase * winSamples;
        const float dl2 = 1.0f + p2 * winSamples;

        y0 = d[0].read (dl1) * g1 + d[0].read (dl2) * g2;
        y1 = d[1].read (dl1) * g1 + d[1].read (dl2) * g2;
    }
};

struct TapeStop
{
    enum State { Play, Slowing, Hold, Recovering };

    std::vector<float> buf[2];
    int size = 8;
    int w = 0;
    State state = Play;
    float readPos = 0.0f;
    float speed = 1.0f;
    int timer = 0;
    double sr = 44100.0;

    void prepare (double s)
    {
        sr = s;
        size = (int) (s * 4.0);
        buf[0].assign ((size_t) size, 0.0f);
        buf[1].assign ((size_t) size, 0.0f);
        reset();
    }

    void reset()
    {
        w = 0; state = Play; speed = 1.0f; timer = 0; readPos = 0.0f;
    }

    void process (float* c0, float* c1, int n, float a)
    {
        float* c[2] = { c0, c1 };
        const int period = (int) (sr * (7.0f - 4.0f * a));
        const int slowLen = std::max (1, (int) (sr * (0.9f - 0.3f * a)));
        const int holdLen = (int) (sr * 0.35f);
        const int recLen = std::max (1, (int) (sr * 0.25f));

        for (int i = 0; i < n; ++i)
        {
            buf[0][(size_t) w] = c[0][i];
            buf[1][(size_t) w] = c[1][i];

            switch (state)
            {
                case Play:
                    if (++timer >= period)
                    {
                        state = Slowing; timer = 0; readPos = (float) w; speed = 1.0f;
                    }
                    break;
                case Slowing:
                    ++timer;
                    speed = 1.0f - (float) timer / (float) slowLen;
                    if (speed <= 0.0f) { speed = 0.0f; state = Hold; timer = 0; }
                    break;
                case Hold:
                    if (++timer >= holdLen) { state = Recovering; timer = 0; }
                    break;
                case Recovering:
                    ++timer;
                    speed = std::min (1.8f, 1.8f * (float) timer / (float) recLen);
                    break;
            }

            if (state != Play)
            {
                int i0 = (int) readPos;
                if (i0 >= size) i0 = 0;
                const float f = readPos - (float) i0;
                int i1 = i0 + 1;
                if (i1 >= size) i1 = 0;
                const float g = std::min (1.0f, speed);

                for (int ch = 0; ch < 2; ++ch)
                {
                    const float s0 = buf[ch][(size_t) i0];
                    const float s1 = buf[ch][(size_t) i1];
                    c[ch][i] = (s0 + (s1 - s0) * f) * g;
                }

                readPos += speed;
                if (readPos >= (float) size) readPos -= (float) size;

                if (state == Recovering)
                {
                    float lag = (float) w - readPos;
                    if (lag < 0.0f) lag += (float) size;
                    if (lag < 2.0f) { state = Play; timer = 0; }
                }
            }

            if (++w >= size) w = 0;
        }
    }
};

struct Granular
{
    static constexpr int kMaxGrains = 12;

    struct Grain
    {
        bool on = false;
        float pos = 0.0f, inc = 1.0f;
        int age = 0, len = 1;
    };

    std::vector<float> buf[2];
    int size = 8;
    int w = 0;
    double sr = 44100.0;
    Grain grains[kMaxGrains];
    int spawnCounter = 0;
    Rng rng;

    void prepare (double s)
    {
        sr = s;
        size = (int) (s * 3.0);
        buf[0].assign ((size_t) size, 0.0f);
        buf[1].assign ((size_t) size, 0.0f);
        w = 0;
        spawnCounter = 0;
        for (auto& g : grains) g.on = false;
    }

    void spawn()
    {
        static const float ratios[5] = { 1.0f, 1.0f, 0.5f, 2.0f, 1.5f };
        for (auto& g : grains)
        {
            if (g.on) continue;
            const float len = rng.range (0.07f, 0.16f) * (float) sr;
            const float inc = ratios[(int) (rng.next() * 5.0f) % 5];
            const float minOff = len * inc + 0.05f * (float) sr;
            const float maxOff = 2.5f * (float) sr;
            const float off = rng.range (minOff, std::max (minOff + 1.0f, maxOff));
            float pos = (float) w - off;
            if (pos < 0.0f) pos += (float) size;
            g.pos = pos; g.inc = inc; g.age = 0; g.len = std::max (1, (int) len); g.on = true;
            return;
        }
    }

    void process (float* c0, float* c1, int n, float a, bool freeze)
    {
        const int interval = std::max (1, (int) (0.035f * (float) sr * (1.6f - a)));
        for (int i = 0; i < n; ++i)
        {
            const float inL = c0[i], inR = c1[i];
            if (! freeze)
            {
                buf[0][(size_t) w] = inL;
                buf[1][(size_t) w] = inR;
                if (++w >= size) w = 0;
            }

            if (--spawnCounter <= 0)
            {
                spawn();
                spawnCounter = interval + (int) (rng.next() * (float) interval * 0.5f);
            }

            float oL = 0.0f, oR = 0.0f;
            for (auto& g : grains)
            {
                if (! g.on) continue;
                const float win = 0.5f - 0.5f * std::cos (kTwoPi * (float) g.age / (float) g.len);
                int i0 = (int) g.pos;
                if (i0 >= size) i0 = 0;
                const float f = g.pos - (float) i0;
                int i1 = i0 + 1;
                if (i1 >= size) i1 = 0;
                oL += (buf[0][(size_t) i0] + (buf[0][(size_t) i1] - buf[0][(size_t) i0]) * f) * win;
                oR += (buf[1][(size_t) i0] + (buf[1][(size_t) i1] - buf[1][(size_t) i0]) * f) * win;
                g.pos += g.inc;
                if (g.pos >= (float) size) g.pos -= (float) size;
                if (++g.age >= g.len) g.on = false;
            }

            const float dryGain = 1.0f - 0.7f * a;
            const float wetGain = 0.55f * (0.4f + 0.6f * a);
            c0[i] = inL * dryGain + oL * wetGain;
            c1[i] = inR * dryGain + oR * wetGain;
        }
    }
};

struct Reverser
{
    std::vector<float> rec[2], play[2];
    int N = 8;
    int pos = 0;
    int fade = 1;

    void prepare (double sr)
    {
        N = std::max (8, (int) (sr * 0.5));
        fade = std::max (1, (int) (sr * 0.02));
        for (int ch = 0; ch < 2; ++ch)
        {
            rec[ch].assign ((size_t) N, 0.0f);
            play[ch].assign ((size_t) N, 0.0f);
        }
        pos = 0;
    }

    void process (float* c0, float* c1, int n, float a)
    {
        float* c[2] = { c0, c1 };
        for (int i = 0; i < n; ++i)
        {
            const int rp = N - 1 - pos;
            const float f = (float) std::min (std::min (pos, N - 1 - pos), fade) / (float) fade;
            for (int ch = 0; ch < 2; ++ch)
            {
                const float x = c[ch][i];
                rec[ch][(size_t) pos] = x;
                const float rev = play[ch][(size_t) rp] * f;
                c[ch][i] = x * (1.0f - 0.6f * a) + rev * a;
            }
            if (++pos >= N)
            {
                pos = 0;
                std::swap (rec[0], play[0]);
                std::swap (rec[1], play[1]);
            }
        }
    }
};

struct Stutter
{
    std::vector<float> hist[2], slice[2];
    int hsize = 8, w = 0;
    int sliceLen = 1, slicePos = 0, repeatsLeft = 0;
    bool active = false;
    int timer = 0, nextTrigger = 1;
    double sr = 44100.0;
    Rng rng;

    void prepare (double s)
    {
        sr = s;
        hsize = std::max (8, (int) (s * 0.6));
        for (int ch = 0; ch < 2; ++ch)
        {
            hist[ch].assign ((size_t) hsize, 0.0f);
            slice[ch].assign ((size_t) ((int) (s * 0.25) + 8), 0.0f);
        }
        reset();
    }

    void reset()
    {
        w = 0; active = false; timer = 0; slicePos = 0; repeatsLeft = 0;
        nextTrigger = (int) (sr * 1.0);
    }

    void process (float* c0, float* c1, int n, float a)
    {
        float* c[2] = { c0, c1 };
        static const float lens[4] = { 0.04f, 0.08f, 0.12f, 0.18f };
        const int fadeS = std::max (1, (int) (sr * 0.002));

        for (int i = 0; i < n; ++i)
        {
            hist[0][(size_t) w] = c[0][i];
            hist[1][(size_t) w] = c[1][i];
            if (++w >= hsize) w = 0;

            if (! active)
            {
                if (++timer >= nextTrigger)
                {
                    active = true;
                    timer = 0;
                    sliceLen = std::max (1, (int) (lens[(int) (rng.next() * 4.0f) % 4] * (float) sr));
                    for (int k = 0; k < sliceLen; ++k)
                    {
                        const int idx = (w - sliceLen + k + hsize) % hsize;
                        slice[0][(size_t) k] = hist[0][(size_t) idx];
                        slice[1][(size_t) k] = hist[1][(size_t) idx];
                    }
                    slicePos = 0;
                    repeatsLeft = 3 + (int) (rng.next() * 6.0f);
                }
            }

            if (active)
            {
                const float e = (float) std::min (std::min (slicePos, sliceLen - 1 - slicePos), fadeS) / (float) fadeS;
                c[0][i] = slice[0][(size_t) slicePos] * e;
                c[1][i] = slice[1][(size_t) slicePos] * e;
                if (++slicePos >= sliceLen)
                {
                    slicePos = 0;
                    if (--repeatsLeft <= 0)
                    {
                        active = false;
                        timer = 0;
                        nextTrigger = (int) (sr * rng.range (0.3f, 3.0f - 2.2f * a));
                    }
                }
            }
        }
    }
};
// ---------------------------------------------------------------------------
// Efectos nuevos
// ---------------------------------------------------------------------------

struct SVF
{
    float s1 = 0.0f, s2 = 0.0f;
    float g = 0.1f, k = 1.0f, a1 = 1.0f;

    void set (float fc, float q, double sr)
    {
        fc = std::min (std::max (fc, 20.0f), (float) (0.45 * sr));
        g = std::tan (3.14159265f * fc / (float) sr);
        k = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
    }

    inline void tick (float x, float& lp, float& bp, float& hp)
    {
        hp = (x - (k + g) * s1 - s2) * a1;
        bp = g * hp + s1;
        lp = g * bp + s2;
        s1 = g * hp + bp;
        s2 = g * bp + lp;
    }

    void reset() { s1 = s2 = 0.0f; }
};

inline float clampBpm (double bpm, float lo, float hi)
{
    return std::min (std::max ((float) bpm, lo), hi);
}

struct Tremolo
{
    float ph = 0.0f;
    double sr = 44100.0;

    void prepare (double s) { sr = s; ph = 0.0f; }

    void process (float* c0, float* c1, int n, float a)
    {
        const float inc = kTwoPi * (3.0f + 6.0f * a) / (float) sr;
        const float depth = 0.3f + 0.7f * a;
        for (int i = 0; i < n; ++i)
        {
            ph += inc;
            if (ph > kTwoPi) ph -= kTwoPi;
            const float g = 1.0f - depth * (0.5f - 0.5f * std::sin (ph));
            c0[i] *= g;
            c1[i] *= g;
        }
    }
};

struct AutoPan
{
    float ph = 0.0f;
    double sr = 44100.0;

    void prepare (double s) { sr = s; ph = 0.0f; }

    void process (float* c0, float* c1, int n, float a)
    {
        const float inc = kTwoPi * (0.3f + 2.2f * a) / (float) sr;
        const float depth = std::min (1.0f, a * 1.5f);
        for (int i = 0; i < n; ++i)
        {
            ph += inc;
            if (ph > kTwoPi) ph -= kTwoPi;
            const float p = 0.5f + 0.5f * depth * std::sin (ph);
            const float gl = 1.4142f * std::cos (p * 1.5708f);
            const float gr = 1.4142f * std::sin (p * 1.5708f);
            c0[i] *= gl;
            c1[i] *= gr;
        }
    }
};

struct Gater
{
    float stepPos = 0.0f;
    int step = 0;
    float env = 1.0f;
    double sr = 44100.0;

    void prepare (double s) { sr = s; stepPos = 0.0f; step = 0; env = 1.0f; }

    void process (float* c0, float* c1, int n, float a, double bpm)
    {
        static const float pat[16] = { 1, 0, 1, 1,  0, 1, 0, 1,  1, 1, 0, 1,  0, 1, 1, 0 };
        const float stepsPerSample = clampBpm (bpm, 40.0f, 300.0f) / 60.0f * 4.0f / (float) sr;
        const float coef = 1.0f - std::exp (-1.0f / (0.002f * (float) sr));

        for (int i = 0; i < n; ++i)
        {
            stepPos += stepsPerSample;
            if (stepPos >= 1.0f) { stepPos -= 1.0f; step = (step + 1) & 15; }
            const float target = 1.0f - a * (1.0f - pat[step]);
            env += (target - env) * coef;
            c0[i] *= env;
            c1[i] *= env;
        }
    }
};

struct Crusher
{
    float hold[2] = { 0.0f, 0.0f };
    float cnt = 0.0f;

    void prepare (double) { hold[0] = hold[1] = 0.0f; cnt = 0.0f; }

    void process (float* c0, float* c1, int n, float a)
    {
        const float factor = 1.0f + a * a * 30.0f;
        const float levels = std::pow (2.0f, 12.0f - 8.0f * a);
        const float mix = std::min (1.0f, a * 2.0f);

        for (int i = 0; i < n; ++i)
        {
            cnt += 1.0f;
            if (cnt >= factor)
            {
                cnt -= factor;
                hold[0] = c0[i];
                hold[1] = c1[i];
            }
            const float q0 = std::round (hold[0] * levels) / levels;
            const float q1 = std::round (hold[1] * levels) / levels;
            c0[i] = c0[i] * (1.0f - mix) + q0 * mix;
            c1[i] = c1[i] * (1.0f - mix) + q1 * mix;
        }
    }
};

struct TapeSat
{
    float lp[2] = { 0.0f, 0.0f };
    double sr = 44100.0;

    void prepare (double s) { sr = s; lp[0] = lp[1] = 0.0f; }

    void process (float* c0, float* c1, int n, float a)
    {
        float* c[2] = { c0, c1 };
        const float drive = 1.0f + 2.5f * a;
        const float comp = 1.0f / std::sqrt (drive);
        const float fc = 12000.0f - 7000.0f * a;
        const float k = 1.0f - std::exp (-kTwoPi * fc / (float) sr);
        const float mix = std::min (1.0f, a * 2.0f);

        for (int i = 0; i < n; ++i)
            for (int ch = 0; ch < 2; ++ch)
            {
                const float x = c[ch][i];
                const float s = std::tanh (x * drive) * comp;
                lp[ch] += k * (s - lp[ch]);
                c[ch][i] = x * (1.0f - mix) + lp[ch] * mix;
            }
    }
};

struct Vinyl
{
    Rng rng;
    float lp[2] = { 0.0f, 0.0f };
    float hpS[2] = { 0.0f, 0.0f };
    float pop = 0.0f;
    double sr = 44100.0;

    void prepare (double s)
    {
        sr = s;
        lp[0] = lp[1] = hpS[0] = hpS[1] = 0.0f;
        pop = 0.0f;
    }

    void process (float* c0, float* c1, int n, float a)
    {
        float* c[2] = { c0, c1 };
        const float klp = 1.0f - std::exp (-kTwoPi * (9000.0f - 5000.0f * a) / (float) sr);
        const float khp = 1.0f - std::exp (-kTwoPi * (60.0f + 120.0f * a) / (float) sr);
        const float p = (1.0f + 25.0f * a) / (float) sr;
        const float popAmt = 0.05f + 0.30f * a;

        for (int i = 0; i < n; ++i)
        {
            if (rng.next() < p)
                pop = rng.range (0.3f, 1.0f) * popAmt * (rng.next() < 0.5f ? -1.0f : 1.0f);
            pop *= 0.92f;
            const float hiss = (rng.next() - 0.5f) * 0.012f * a;

            for (int ch = 0; ch < 2; ++ch)
            {
                float x = c[ch][i];
                lp[ch] += klp * (x - lp[ch]);
                hpS[ch] += khp * (lp[ch] - hpS[ch]);
                c[ch][i] = (lp[ch] - hpS[ch]) + pop * (ch == 0 ? 1.0f : 0.8f) + hiss;
            }
        }
    }
};

struct AutoWah
{
    SVF f[2];
    float ph = 0.0f;
    double sr = 44100.0;

    void prepare (double s) { sr = s; ph = 0.0f; f[0].reset(); f[1].reset(); }

    void process (float* c0, float* c1, int n, float a)
    {
        float* c[2] = { c0, c1 };
        const float inc = kTwoPi * (0.3f + 2.2f * a) / (float) sr;
        const float mix = std::min (1.0f, a * 1.5f);

        for (int i = 0; i < n; ++i)
        {
            ph += inc;
            if (ph > kTwoPi) ph -= kTwoPi;
            const float fc = 350.0f * std::pow (2.0f, 3.0f * (0.5f + 0.5f * std::sin (ph)));
            f[0].set (fc, 3.5f, sr);
            f[1].g = f[0].g; f[1].k = f[0].k; f[1].a1 = f[0].a1;

            for (int ch = 0; ch < 2; ++ch)
            {
                float lp, bp, hp;
                f[ch].tick (c[ch][i], lp, bp, hp);
                const float wet = bp * f[ch].k * 2.0f;
                c[ch][i] = c[ch][i] * (1.0f - 0.7f * mix) + wet * mix;
            }
        }
    }
};

struct PingPong
{
    DelayBuf d[2];
    double sr = 44100.0;
    bool idle = true;

    void prepare (double s)
    {
        sr = s;
        for (auto& x : d) x.prepare ((int) (s * 0.5));
        idle = true;
    }

    void markIdle()
    {
        if (idle) return;
        for (auto& x : d) x.buf.assign (x.buf.size(), 0.0f);
        idle = true;
    }

    void process (float* c0, float* c1, int n, float a)
    {
        idle = false;
        const float dt = 0.34f * (float) sr;
        const float fb = 0.35f + 0.35f * a;
        const float wet = 0.4f + 0.5f * a;

        for (int i = 0; i < n; ++i)
        {
            const float inL = c0[i], inR = c1[i];
            const float dl = d[0].read (dt);
            const float dr = d[1].read (dt);
            d[0].push ((inL + inR) * 0.5f + dr * fb);
            d[1].push (dl * fb);
            c0[i] = inL + dl * wet;
            c1[i] = inR + dr * wet;
        }
    }
};

struct Slicer
{
    DelayBuf d[2];
    Rng rng;
    float stepPos = 0.0f;
    float delaySamp[2] = { 1.0f, 1.0f };
    float gain[2] = { 0.0f, 0.0f };
    float target[2] = { 0.0f, 0.0f };
    int cur = 0;
    bool idle = true;
    double sr = 44100.0;

    void prepare (double s)
    {
        sr = s;
        for (auto& x : d) x.prepare ((int) (s * 4.0));
        stepPos = 0.0f; cur = 0; idle = true;
        gain[0] = gain[1] = target[0] = target[1] = 0.0f;
    }

    void markIdle()
    {
        if (idle) return;
        for (auto& x : d) x.buf.assign (x.buf.size(), 0.0f);
        gain[0] = gain[1] = target[0] = target[1] = 0.0f;
        idle = true;
    }

    void process (float* c0, float* c1, int n, float a, double bpm)
    {
        idle = false;
        const float stepLen = (float) sr * 60.0f / clampBpm (bpm, 60.0f, 200.0f) * 0.5f;
        const float stepsPerSample = 1.0f / stepLen;
        const float coef = 1.0f - std::exp (-1.0f / (0.004f * (float) sr));

        for (int i = 0; i < n; ++i)
        {
            stepPos += stepsPerSample;
            if (stepPos >= 1.0f)
            {
                stepPos -= 1.0f;
                if (rng.next() < 0.15f + 0.6f * a)
                {
                    const int k = 1 + (int) (rng.next() * 5.0f) % 5;
                    cur ^= 1;
                    delaySamp[cur] = (float) k * stepLen;
                    target[cur] = 1.0f;
                    target[cur ^ 1] = 0.0f;
                }
                else
                {
                    target[0] = target[1] = 0.0f;
                }
            }

            const float x0 = c0[i], x1 = c1[i];
            d[0].push (x0);
            d[1].push (x1);

            gain[0] += (target[0] - gain[0]) * coef;
            gain[1] += (target[1] - gain[1]) * coef;

            const float dryG = std::max (0.0f, 1.0f - gain[0] - gain[1]);
            float o0 = x0 * dryG, o1 = x1 * dryG;
            for (int h = 0; h < 2; ++h)
            {
                if (gain[h] > 0.0005f)
                {
                    o0 += d[0].read (delaySamp[h]) * gain[h];
                    o1 += d[1].read (delaySamp[h]) * gain[h];
                }
            }
            c0[i] = o0;
            c1[i] = o1;
        }
    }
};

struct Unison
{
    PitchShifter a, b;

    void prepare (double sr) { a.prepare (sr); b.prepare (sr); }

    void process (float* c0, float* c1, int n, float amt)
    {
        const float cents = 6.0f + 14.0f * amt;
        const float rUp = std::pow (2.0f, cents / 1200.0f);
        const float rDn = std::pow (2.0f, -cents / 1200.0f);
        const float m = std::min (1.0f, amt * 1.5f);

        for (int i = 0; i < n; ++i)
        {
            const float x0 = c0[i], x1 = c1[i];
            float ua0, ua1, ub0, ub1;
            a.processSample (x0, x1, rUp, ua0, ua1);
            b.processSample (x0, x1, rDn, ub0, ub1);
            c0[i] = x0 * (1.0f - 0.4f * m) + ua0 * 0.5f * m + ub0 * 0.15f * m;
            c1[i] = x1 * (1.0f - 0.4f * m) + ub1 * 0.5f * m + ua1 * 0.15f * m;
        }
    }
};

struct Phone
{
    SVF hp[2], lp[2];
    double sr = 44100.0;

    void prepare (double s)
    {
        sr = s;
        for (int ch = 0; ch < 2; ++ch) { hp[ch].reset(); lp[ch].reset(); }
    }

    void process (float* c0, float* c1, int n, float a)
    {
        float* c[2] = { c0, c1 };
        const float mix = std::min (1.0f, a * 1.3f);
        for (int ch = 0; ch < 2; ++ch)
        {
            hp[ch].set (300.0f + 250.0f * a, 0.8f, sr);
            lp[ch].set (3600.0f - 1200.0f * a, 0.8f, sr);
        }

        for (int i = 0; i < n; ++i)
            for (int ch = 0; ch < 2; ++ch)
            {
                const float x = c[ch][i];
                float l1, b1, h1, l2, b2, h2;
                hp[ch].tick (x, l1, b1, h1);
                lp[ch].tick (h1, l2, b2, h2);
                const float y = std::tanh (1.4f * l2) * 1.1f;
                c[ch][i] = x * (1.0f - mix) + y * mix;
            }
    }
};
} // namespace mfx
