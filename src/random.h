#ifndef XOSHIRO_H
#define XOSHIRO_H

#include "lib_includes.h"


namespace xo  // short for xoshiro
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
        for (int i = 0; i < 4; ++i) {
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


// Use a function to return a reference to the thread_local generator
// This avoids linker issues with inline thread_local variables
inline xoshiro256pp& get_gen() {
    static thread_local xoshiro256pp gen{};
    return gen;
}

// Use this if you want to seed with a specific value
inline void seed(uint64_t s) {
    get_gen() = xoshiro256pp{s};
}

inline int get(int min, int max) {
    return std::uniform_int_distribution{min, max}(get_gen());
}

// inline std::vector<int> get_n_draws(int min, int max, int n) {
//   std::vector<int> peeps;
//   peeps.reserve(n);
//   std::uniform_int_distribution<int> dist{min, max};  // Create once
//   for (int i = 0; i < n; ++i) {
//     peeps.push_back(dist(get_gen()));
//   }
//   return peeps;
// }

// updates passed in buffer reference in place
template<typename IntType = int>
void get_n_draws(IntType min, IntType max, int n, std::vector<IntType> &buffer) {
  buffer.clear();
  std::uniform_int_distribution<IntType> dist{min, max};
  for (int i = 0; i < n; ++i) {
    buffer.push_back(dist(get_gen()));
  }
}


// returns vector result:  don't know if we'll use
template<typename IntType = int>
std::vector<IntType> get_n_draws(IntType min, IntType max, int n) {
  std::vector<IntType> buffer;
  buffer.reserve(n);  // one allocation
  std::uniform_int_distribution<IntType> dist{min, max};
  for (int i = 0; i < n; ++i) {
    buffer.push_back(dist(get_gen()));
  }
  return buffer;
}



inline int gamma_int(double shape, double scale, int max_value = 50) {
    double value = std::gamma_distribution<double>{shape, scale}(get_gen());
    return std::clamp(static_cast<int>(std::round(value)), 0, max_value);
}

inline bool bernoulli(double p) {
    return std::bernoulli_distribution{p}(get_gen());
}

inline int categorical_uniform(int k) {
    return std::uniform_int_distribution{0, k - 1}(get_gen());
}

template<typename ReturnType = int, typename Container>
inline ReturnType categorical_fast(const Container& probs) {
    using FloatType = typename Container::value_type;
    FloatType sum = FloatType{0};
    for (auto p : probs) sum += p;
    const FloatType tol = std::sqrt(std::numeric_limits<FloatType>::epsilon());
    if (std::abs(sum - FloatType{1}) > tol) {
        return static_cast<ReturnType>(0);  // matches Julia: return 0 for invalid distribution
    }

    FloatType u = std::uniform_real_distribution<FloatType>{}(get_gen());
    FloatType cumpr = FloatType{0};
    for (size_t i = 0; i < probs.size(); ++i) {
        cumpr += probs[i];
        if (u <= cumpr) {
            return static_cast<ReturnType>(i);
        }
    }
    return static_cast<ReturnType>(probs.size() - 1);
}

}

#endif