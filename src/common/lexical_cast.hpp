#ifndef LEXICAL_CAST_HPP_LQUOTFJ9
#define LEXICAL_CAST_HPP_LQUOTFJ9

#include <sstream>

template <typename T>
inline T lexical_cast(const std::string& str)
{
    std::istringstream iss{str};
    T value;

    iss >> value;

    return value;
}

template <>
inline std::string lexical_cast(const std::string& str)
{
    return str;
}

template <typename T>
inline std::string lexical_cast(const T& value)
{
    std::ostringstream oss;
    oss << value;

    return oss.str();
}

#endif /* end of include guard: LEXICAL_CAST_HPP_LQUOTFJ9 */
