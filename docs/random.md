# Approaches to using random number generation in c++

I have the following random code suggested by learncpp.  at base, it's good enough.

I have the following use cases for random numbers:
1. generate k random ints from a range that goes from 1 to n where n is much larger than k.  we could instantiate such an array, but perhaps it is not necessary.
2. generate a draw from a binomial distribution with a random result of 0 or 1, given an input probability of p in [0.0, 1.0].  this could probably be done more efficiently using the uniform dist. PRNG and returning 1 for the PRNG output <= p and 0 otherwise.
3. simulate draws from a categorical distribution, which is generally for 5 categories, 5 binomial disttributions with the same probability per category summing to 1.0, or unequal probs to get some sort of a priori weighting. For the equally weighted case, it is probably easiest to generate a draw from 1 to k for k categories using the uniform PRNG below.  
4. generate an integer from 0 to as much as 12 based on a gamma distribution.  gamma is desired because the outcomes shoulkd not be symmetrical.  we want a clump near zero and a low prob. tail out towards 12. In Julia, the shape of the gamma dist is provided by parameters shape and scale.  I did a bunch of runs and plotted the outcomes and chose scale and shape for outcomes I thought were reasonable. the julia code is also below.

Here's how to extend that Random namespace for your use cases:
```cpp
#ifndef RANDOM_MT_H
#define RANDOM_MT_H

#include <chrono>
#include <random>
#include <unordered_set>
#include <vector>
#include <algorithm>

namespace Random
{
    // ... [existing code] ...

    // Use case 1: k unique random ints from [min, max] where range >> k
    // Uses repeated sampling with unordered_set to ensure uniqueness
    inline std::vector<int> get_unique(int min, int max, int k)
    {
        std::unordered_set<int> values;
        values.reserve(k);
        
        while (values.size() < static_cast<size_t>(k)) {
            values.insert(std::uniform_int_distribution{min, max}(mt));
        }
        
        return std::vector<int>(values.begin(), values.end());
    }

    // Use case 2: Bernoulli trial (0 or 1) with probability p
    // Your approach is fine, but std::bernoulli_distribution exists
    inline int bernoulli(double p)
    {
        return std::bernoulli_distribution{p}(mt);
    }
    
    // Or if you really want the manual version:
    inline int bernoulli_manual(double p)
    {
        return (std::uniform_real_distribution{0.0, 1.0}(mt) <= p) ? 1 : 0;
    }

    // Use case 3: Categorical distribution with k categories
    // Equal probabilities - just uniform int
    inline int categorical_uniform(int k)
    {
        return std::uniform_int_distribution{0, k - 1}(mt);
    }
    
    // Unequal probabilities - std::discrete_distribution handles this
    inline int categorical(const std::vector<double>& probabilities)
    {
        return std::discrete_distribution<int>{probabilities.begin(), probabilities.end()}(mt);
    }

    // Use case 4: Integer from gamma distribution, clamped to [0, max_value]
    inline int gamma_int(double shape, double scale, int max_value = 12)
    {
        double value = std::gamma_distribution<double>{shape, scale}(mt);
        int result = static_cast<int>(std::round(value));
        return std::clamp(result, 0, max_value);
    }
}

#endif
```

For your Julia how_many_contacts function, the C++ equivalent would be:
```cpp
inline int how_many_contacts(double density_factor, double indoor_factor, 
                             double gammashape, double scale_base)
{
    double scale = density_factor * indoor_factor * scale_base;
    return Random::gamma_int(gammashape, scale, 12);
}
```

Here's how to extend that Random namespace for your use cases:

```cpp
#ifndef RANDOM_MT_H
#define RANDOM_MT_H

#include <chrono>
#include <random>
#include <unordered_set>
#include <vector>
#include <algorithm>

namespace Random
{
    // ... [existing code] ...

    // Use case 1: k unique random ints from [min, max] where range >> k
    // Uses repeated sampling with unordered_set to ensure uniqueness
    inline std::vector<int> get_unique(int min, int max, int k)
    {
        std::unordered_set<int> values;
        values.reserve(k);
        
        while (values.size() < static_cast<size_t>(k)) {
            values.insert(std::uniform_int_distribution{min, max}(mt));
        }
        
        return std::vector<int>(values.begin(), values.end());
    }

    // Use case 2: Bernoulli trial (0 or 1) with probability p
    // Your approach is fine, but std::bernoulli_distribution exists
    inline int bernoulli(double p)
    {
        return std::bernoulli_distribution{p}(mt);
    }
    
    // Or if you really want the manual version:
    inline int bernoulli_manual(double p)
    {
        return (std::uniform_real_distribution{0.0, 1.0}(mt) <= p) ? 1 : 0;
    }

    // Use case 3: Categorical distribution with k categories
    // Equal probabilities - just uniform int
    inline int categorical_uniform(int k)
    {
        return std::uniform_int_distribution{0, k - 1}(mt);
    }
    
    // Unequal probabilities - std::discrete_distribution handles this
    inline int categorical(const std::vector<double>& probabilities)
    {
        return std::discrete_distribution<int>{probabilities.begin(), probabilities.end()}(mt);
    }

    // Use case 4: Integer from gamma distribution, clamped to [0, max_value]
    inline int gamma_int(double shape, double scale, int max_value = 12)
    {
        double value = std::gamma_distribution<double>{shape, scale}(mt);
        int result = static_cast<int>(std::round(value));
        return std::clamp(result, 0, max_value);
    }
}

#endif
```

**Key points:**

1. **Unique sampling**: The unordered_set approach works well when k << n. If k is close to n, you'd want Fisher-Yates shuffle instead, but that requires the array you wanted to avoid.

2. **Bernoulli**: `std::bernoulli_distribution` is standard, but benchmarking might show your manual approach is faster since it avoids the distribution object overhead. Profile if this is in a hot path.

3. **Categorical**: `std::discrete_distribution` is the right tool. For the equal-probability case, just use uniform int. Don't overthink it.

4. **Gamma**: `std::gamma_distribution` takes shape and scale like Julia's Gamma(). The clamping ensures you stay in [0, 12]. If you're calling this millions of times with the same shape/scale, consider caching the distribution object rather than recreating it.

For your Julia `how_many_contacts` function, the C++ equivalent would be:

```cpp
inline int how_many_contacts(double density_factor, double indoor_factor, 
                             double gammashape, double scale_base)
{
    double scale = density_factor * indoor_factor * scale_base;
    return Random::gamma_int(gammashape, scale, 12);
}
```

One thing to watch: if you're creating/destroying distribution objects in tight loops (millions of times), profile it. The distribution objects have state, and construction isn't free. If scale varies continuously, you're stuck with it. If scale has limited distinct values, cache the distributions in a map.

I will point out that my cum probability hack for categorical was 10x faster (1/10 of execution itme) for Julia's code using a "real" categorical distribution.  will c++'s discrete distribution have a similar perf problem?

You're absolutely right to be skeptical. For 5 categories, std::discrete_distribution is almost certainly overkill and slower than your cumulative probability approach.
std::discrete_distribution typically uses Walker's alias method or similar, which has O(1) sampling after O(n) preprocessing. But that preprocessing cost hits you on every construction, and even the "O(1)" sampling has more overhead than a simple linear scan for 5 elements.
Here's your cumulative probability hack in C++:
```cpp
// Pre-compute cumulative probabilities once, store them
inline int categorical_fast(const std::vector<double>& cum_probs)
{
    double u = std::uniform_real_distribution{0.0, 1.0}(mt);
    
    // Linear search for 5 categories - faster than anything fancy
    for (size_t i = 0; i < cum_probs.size(); ++i) {
        if (u <= cum_probs[i]) {
            return static_cast<int>(i);
        }
    }
    return static_cast<int>(cum_probs.size() - 1); // shouldn't happen, but handles fp precision
}

// Or if you want to pass raw probabilities and compute cumsum inline
inline int categorical_fast(const double* probs, int n)
{
    double u = std::uniform_real_distribution{0.0, 1.0}(mt);
    double cumsum = 0.0;
    
    for (int i = 0; i < n; ++i) {
        cumsum += probs[i];
        if (u <= cumsum) {
            return i;
        }
    }
    return n - 1;
}
```

For my k outcomes case I prefer to use Gamma, as noted becasuse Binomial while not symmetrically  is to "central" compared to the long tail of suitable gamma distributions.

Right, gamma is the correct choice there. Binomial(n=12, p) would peak around 12*p and have symmetric-ish tails (especially for moderate p). You can't get the right-skewed, zero-clumped, long-tail behavior you want.

Gamma with low shape parameter (say shape < 2) gives you exactly that: mass piled up near zero, then a long tail stretching out. As you tune shape and scale, you control how quickly that tail decays.

For something like contact counts where you want:
- Most days: 0-2 contacts
- Occasionally: 5-8 contacts  
- Rarely: 10-12 contacts

Gamma is perfect. Binomial would give you too many outcomes in the middle range.

Your Julia approach of running trials, plotting the distributions, and picking shape/scale that "look right" is the sensible way to do it. The C++ `std::gamma_distribution` will behave identically with the same parameters.