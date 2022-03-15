#pragma once

#include <functional>

/**
 * @brief Utitlity to create hash function of complex user-defined types.
 *
 * @param seed  agregated hash value and initial seed
 * @param v     value to add the hash result
 *
 * Can bu used like this:
 * @code
 * struct ItemHasher
 * {
 *     std::size_t operator()(Item const& item) const
 *     {
 *         std::size_t seed = 0;
 *         hash_combine(seed, item.name);
 *         hash_combine(seed, item.age);
 *         hash_combine(seed, item.code);
 *         return seed;
 *     }
 * };
 * @endcode
 *
 * @note Courtesy of Nam Seob Seo:
 * https://acrocontext.wordpress.com/2014/01/30/c-hash-combine-example-for-hash-function-in-unordered_map/
 */
template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
