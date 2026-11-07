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
} // namespace mfx
