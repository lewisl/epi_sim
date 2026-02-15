#ifndef XOSHIRO_H
#define XOSHIRO_H

#include "lib_includes.h"


namespace Xoshiro
{
class xoshiro256pp {
private:
    uint64_t s[4];
    
    static inline uint64_t rotl(const uint64_t x, int k) {
        return (x << k) | (x >> (64 - k));
    }

public:
    using result_type = uint64_t;
    
    xoshiro256pp() {
        std::random_device rd;
        s[0] = (uint64_t(rd()) << 32) | rd();
        s[1] = (uint64_t(rd()) << 32) | rd();
        s[2] = (uint64_t(rd()) << 32) | rd();
        s[3] = (uint64_t(rd()) << 32) | rd();
    }

    xoshiro256pp(uint64_t seed) {
        uint64_t z = seed;
        for (int i = 0; i < 4; i++) {
            z += 0x9e3779b97f4a7c15;
            uint64_t temp = z;
            temp = (temp ^ (temp >> 30)) * 0xbf58476d1ce4e5b9;
            temp = (temp ^ (temp >> 27)) * 0x94d049bb133111eb;
            s[i] = temp ^ (temp >> 31);
        }
    }
    
    static constexpr result_type min() { return 0; }
    static constexpr result_type max() { return UINT64_MAX; }
    
    result_type operator()() {
        const uint64_t result = rotl(s[0] + s[3], 23) + s[0];
        const uint64_t t = s[1] << 17;
        
        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];
        s[2] ^= t;
        s[3] = rotl(s[3], 45);
        
        return result;
    }
};


inline thread_local xoshiro256pp gen{};

// Use this if you want to seed with a specific value
inline void seed(uint64_t s) {
    gen = xoshiro256pp{s};
}

inline int get(int min, int max) {
    return std::uniform_int_distribution{min, max}(gen);
}

inline int gamma_int(double shape, double scale, int max_value = 12) {
    double value = std::gamma_distribution<double>{shape, scale}(gen);
    return std::clamp(static_cast<int>(std::round(value)), 0, max_value);
}

inline int bernoulli(double p) {
    return std::bernoulli_distribution{p}(gen) ? 1 : 0;
}

inline int categorical_uniform(int k) {
    return std::uniform_int_distribution{0, k - 1}(gen);
}

inline int categorical_fast(const std::vector<double>& cum_probs) {
    double u = std::uniform_real_distribution<double>{0.0, 1.0}(gen);
    
    for (size_t i = 0; i < cum_probs.size(); ++i) {
        if (u <= cum_probs[i]) {
            return static_cast<int>(i);
        }
    }
    return static_cast<int>(cum_probs.size() - 1);
}

}

#endif