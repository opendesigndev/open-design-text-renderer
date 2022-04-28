#pragma once

#include <algorithm>
#include <iterator>
#include <vector>

template <typename T>
class sorted_vector
{
public:
    using vector_type = std::vector<T>;

    sorted_vector() = default;

    /* implicit */ sorted_vector(const vector_type& vec)
        : data_(std::move(vec))
    {
        init();
    }

    /* implicit */ sorted_vector(const vector_type&& vec)
        : data_(vec)
    {
        init();
    }

    sorted_vector<T>& operator=(const sorted_vector<T>&) = default;

    sorted_vector<T>& operator=(const vector_type& rhs)
    {
        *this = sorted_vector<T>(rhs);
        return *this;
    }

    std::size_t size() const
    {
        return data_.size();
    }

    typename vector_type::const_iterator begin() const
    {
        return data_.begin();
    }

    typename vector_type::const_iterator end() const
    {
        return data_.end();
    }

    typename vector_type::const_iterator upper_bound(const T& value) const
    {
        return std::upper_bound(std::begin(data_), std::end(data_), value);
    }

    bool operator==(const sorted_vector<T>& other) const
    {
        return data_ == other.data_;
    }

    const T& operator[](std::size_t idx) const
    {
        return data_[idx];
    }


private:
    void init()
    {
        std::sort(std::begin(data_), std::end(data_));
    }

    vector_type data_;
};

template <typename T>
inline typename sorted_vector<T>::vector_type::const_iterator begin(const sorted_vector<T>& vec)
{
    return vec.begin();
}

template <typename T>
inline typename sorted_vector<T>::vector_type::const_iterator end(const sorted_vector<T>& vec)
{
    return vec.end();
}
