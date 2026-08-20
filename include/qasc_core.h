/**
 * @copyright Copyright (c) 2025-2026
 * UK Research and Innovation,
 * Science and Technology Facilities Council,
 * Hartree Centre
 **/

#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iostream>
#include <numbers>
#include <ranges>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace qasc {

using ssize = ssize_t;
constexpr std::complex<double> operator""_im(long double d) {
    return std::complex<double>{0.0, static_cast<double>(d)};
}

constexpr std::complex<double> operator""_im(unsigned long long d) {
    return std::complex<double>{0.0, static_cast<double>(d)};
}

inline int nextQuregStartIdx = 0;

using std::cos;
using std::exp;
using std::floor;
using std::log;
using std::rotl;
using std::rotr;
using std::sin;
using std::sqrt;
using std::tan;

inline double arcsin(double val) {
    return std::asin(val);
}

inline double arccos(double val) {
    return std::acos(val);
}

inline double arctan(double val) {
    return std::atan(val);
}

inline double ceiling(double val) {
    return std::ceil(val);
}

inline double mod(double val1, double val2) {
    return std::fmod(val1, val2);
}

using std::pow;

constexpr int pow(int base, int exp) {
    int result = 1;
    for (int i = 0; i < exp; ++i) {
        result *= base;
    }
    return result;
}

constexpr double pow(double base, int exp) {
    double result = 1;
    for (int i = 0; i < exp; ++i) {
        result *= base;
    }
    return result;
}

inline double real(std::complex<double> z) {
    return z.real();
}

inline double imag(std::complex<double> z) {
    return z.imag();
}

template <typename T>
struct is_vector : std::false_type {};

template <typename T, typename Alloc>
struct is_vector<std::vector<T, Alloc>> : std::true_type {};

template <typename T>
ssize qasmSizeof(const std::vector<T>& v, int dim_idx = 0) {
    if (dim_idx == 0) {
        return v.size();
    }

    if (v.empty()) {
        return 0;
    }

    if constexpr (is_vector<T>::value) {
        return qasmSizeof(v[0], dim_idx - 1);
    } else {
        return 0;
    }
}

class QuregTag {};

template <typename T>
concept QasmQuregT = std::derived_from<std::remove_reference_t<T>, QuregTag>;

struct QuregSlice : public QuregTag {
    QuregSlice* parentSlice{nullptr};
    int start{0};
    int end{0};
    int increment{1};
    int numQubits{0};

    // NOTE: end is inclusive
    QuregSlice(int start_, int end_, int increment_)
        : start(start_), end(end_ + increment_), increment(increment_) {
        numQubits = (end - start) / increment;
    }

    QuregSlice(int size)
        : start(nextQuregStartIdx),
          end(nextQuregStartIdx + size),
          increment(1) {
        numQubits = size;
        nextQuregStartIdx = end;
    }

    QuregSlice(QuregSlice* parentSlice_, int start_, int end_, int increment_)
        : parentSlice(parentSlice_),
          start(start_),
          end(end_ + increment_),
          increment(increment_) {
        if (parentSlice_ == nullptr) {
            throw "Should not call it with null parent!";
        }
        numQubits = (end - start) / increment;
    }

    QuregSlice(QuregSlice& parentSlice_, int start_, int end_, int increment_)
        : parentSlice(&parentSlice_),
          start(start_),
          end(end_ + increment_),
          increment(increment_) {
        numQubits = (end - start) / increment;
    }

    // types of slices:
    // normal:
    //
    // negative:
    // q: start = 0, end = 6, last idx = 5
    // q[-4:-2] ->  [2, 3, 4]
    //
    // decrement:
    // q[5:-1:2] -> [5, 4, 3, 2]
    //
    //
    QuregSlice slice(int start_, int end_, int increment_ = 1) {
        ssize trueStart = start_ < 0 ? end + start_ : start_;
        ssize trueEnd = end_ < 0 ? end + end_ : end_;
        if (trueEnd < trueStart && increment_ > 0) {
            throw std::runtime_error(
                std::format("end ({}) is smaller than start ({}). increment "
                            "should be negative but "
                            "is positive ({}).",
                            trueEnd, trueStart, increment_));
        }
        return QuregSlice(this, trueStart, trueEnd, increment_);
    }

    QuregSlice operator[](ssize index) {
        ssize trueIdx = index < 0 ? end + index : index;
        return QuregSlice(this, trueIdx, trueIdx, 1);
    }
};

template <typename T>
concept Integer = std::integral<T>;

template <class T>
concept Numeric = std::is_arithmetic_v<T>;

template <typename UserT, unsigned int N = 32, bool isAngle = false>
struct QasmType {
    using i32 = int32_t;
    using i64 = int64_t;
    using u32 = uint32_t;
    using u64 = uint64_t;

    using T = std::conditional_t<
        isAngle,
        double,
        std::conditional_t<std::is_signed_v<UserT>,
                           std::conditional_t<(N > 32), i64, i32>,
                           std::conditional_t<(N > 32), u64, u32>>>;

    // NOTE: running little-endian
    std::array<bool, N> bits{};

    QasmType() = default;

    constexpr QasmType(T value) {
        constexpr auto maxAllowedBits = sizeof(u64) * CHAR_BIT;
        if constexpr (isAngle && N < maxAllowedBits) {
            value = std::fmod(value, 2.0 * std::numbers::pi);
            if (value < 0) {
                value += 2.0 * std::numbers::pi;
            }
            u64 intValue = static_cast<u64>(
                std::floor((value * (static_cast<u64>(1) << N) /
                            (2.0 * std::numbers::pi))));
            for (ssize i = 0; i < N; ++i) {
                bits[i] = static_cast<bool>((intValue >> i) & 1);
            }

        } else {
            for (ssize i = 0; i < N; ++i) {
                bits[i] = static_cast<bool>((value >> i) & 1);
            }
        }
    }

    constexpr explicit QasmType(std::initializer_list<bool> list) : bits(list) {
    }

    constexpr QasmType(const std::string& bitStr) {
        auto workStr = bitStr;
        workStr.erase(std::remove(workStr.begin(), workStr.end(), '_'),
                      workStr.end());

        std::reverse(workStr.begin(), workStr.end());
        if (workStr.size() > N) {
            throw "Failed bit string initialization.";
        }

        for (ssize i = 0; i < workStr.size(); ++i) {
            bits[i] = workStr[i] == '0' ? 0 : 1;
        }
    }

    template <typename V>
    constexpr QasmType(std::vector<V>&& bitStr) {
        if (bitStr.size() > N) {
            throw "Failed bit string initialization.";
        }
        for (ssize i = 0; i < bitStr.size(); ++i) {
            bits[i] = static_cast<bool>(bitStr[i]);
        }
    }

    constexpr operator T() const {
        if constexpr (isAngle) {
            u64 uintResult = 0;
            constexpr auto maxAllowedBits = sizeof(double) * CHAR_BIT;
            if constexpr (N < maxAllowedBits) {
                for (ssize i = 0; i < N; ++i) {
                    if (bits[i]) {
                        uintResult |= (static_cast<u64>(1) << (i));
                    }
                }

                double result = 2.0 * std::numbers::pi * uintResult /
                                static_cast<double>(static_cast<u64>(1) << N);
                return result;
            } else {
                return 2.0 * std::numbers::pi;
            }

        } else {
            T result = 0;
            for (ssize i = 0; i < N; ++i) {
                if (bits[i]) {
                    result |= (static_cast<T>(1) << (i));
                }
            }

            // two's complement
            if constexpr (std::is_signed_v<T>) {
                constexpr auto maxAllowedBits = sizeof(T) * CHAR_BIT;
                if constexpr (N < maxAllowedBits) {
                    if (result && bits[N - 1]) {
                        T signMask = ~((static_cast<T>(1) << N) - 1);
                        result |= signMask;
                    }
                }
            }
            return result;
        }
    }

    constexpr void writeBits(T value) {
        bits.fill(0);
        constexpr auto maxAllowedBits = sizeof(u64) * CHAR_BIT;
        if constexpr (isAngle && N < maxAllowedBits) {
            value = std::fmod(value, 2.0 * std::numbers::pi);
            if (value < 0) {
                value += 2.0 * std::numbers::pi;
            }
            u64 intValue = static_cast<u64>(
                std::floor((value * (static_cast<u64>(1) << N) /
                            (2.0 * std::numbers::pi))));

            for (ssize i = 0; i < N; ++i) {
                bits[i] = static_cast<bool>((intValue >> i) & 1);
            }
        } else {
            for (ssize i = 0; i < N; ++i) {
                bits[i] = static_cast<bool>((value >> i) & 1);
            }
        }
    }

    constexpr bool operator[](ssize index) const {
        // negative index goes from back, i.e.:
        // index = -1 -> trueIdx = N - 1
        ssize trueIdx = index < 0 ? N + index : index;
        return bits[trueIdx];
    }

    typename std::array<bool, N>::reference operator[](ssize index) {
        ssize trueIdx = index < 0 ? N + index : index;
        return bits[trueIdx];
    }

    auto begin() {
        return bits.begin();
    }

    auto begin() const {
        return bits.begin();
    }

    auto end() {
        return bits.end();
    }

    auto end() const {
        return bits.end();
    }

    template <Integer OtherT>
    QasmType& operator=(OtherT&& other) {
        auto otherVal = static_cast<OtherT>(other);
        auto result = otherVal;
        writeBits(result);
        return *this;
    }

    QasmType& operator=(const std::vector<T>&& other) {
        if (other.size() > N) {
            throw std::runtime_error(
                "Attempting to assign variable with more bits than target.");
        }
        for (ssize i = 0; i < other.size(); ++i) {
            bits[i] = other[i];
        }
        return *this;
    }

    template <typename OtherUserT, unsigned int OtherN>
    QasmType& operator=(const QasmType<OtherUserT, OtherN>& other) {
        if (OtherN > N) {
            throw std::runtime_error(
                "Attempting to assign variable with more bits than target.");
        }
        for (ssize i = 0; i < N; ++i) {
            bits[i] = other[i];
        }
        return *this;
    }

    template <typename OtherUserT, unsigned int OtherN>
    QasmType& operator=(const QasmType<OtherUserT, OtherN, true>& other) {
        if (OtherN > N) {
            throw std::runtime_error(
                "Attempting to assign variable with more bits than target.");
        }
        for (ssize i = 0; i < N; ++i) {
            bits[i] = other[i];
        }
        return *this;
    }

    template <typename OtherUserT, unsigned int OtherN>
    constexpr QasmType& operator+=(const QasmType<OtherUserT, OtherN>& rhs) {
        using OtherT = typename QasmType<OtherUserT, OtherN>::T;
        auto lhsVal = static_cast<T>(*this);
        auto rhsVal = static_cast<OtherT>(rhs);
        auto result = lhsVal + rhsVal;
        writeBits(result);
        return *this;
    }

    template <typename OtherUserT, unsigned int OtherN>
    constexpr QasmType& operator-=(const QasmType<OtherUserT, OtherN>& rhs) {
        using OtherT = typename QasmType<OtherUserT, OtherN>::T;
        auto lhsVal = static_cast<T>(*this);
        auto rhsVal = static_cast<OtherT>(rhs);
        auto result = lhsVal - rhsVal;
        writeBits(result);
        return *this;
    }

    template <typename OtherUserT, unsigned int OtherN>
    constexpr QasmType& operator*=(const QasmType<OtherUserT, OtherN>& rhs) {
        using OtherT = typename QasmType<OtherUserT, OtherN>::T;
        auto lhsVal = static_cast<T>(*this);
        auto rhsVal = static_cast<OtherT>(rhs);
        auto result = lhsVal * rhsVal;
        writeBits(result);
        return *this;
    }

    template <typename OtherUserT, unsigned int OtherN>
    constexpr QasmType& operator/=(const QasmType<OtherUserT, OtherN>& rhs) {
        using OtherT = typename QasmType<OtherUserT, OtherN>::T;
        auto lhsVal = static_cast<T>(*this);
        auto rhsVal = static_cast<OtherT>(rhs);
        if (rhsVal == 0) {
            throw "division by zero!";
        };
        auto result = lhsVal / rhsVal;
        writeBits(result);
        return *this;
    }

    template <typename RhsT, unsigned int RhsN, bool isRhsAngle>
    constexpr friend QasmType operator+(
        const QasmType& lhs,
        const QasmType<RhsT, RhsN, isRhsAngle>& rhs) {
        if constexpr (isRhsAngle) {
            if constexpr (N > RhsN) {
                return QasmType<unsigned int, N, true>(QasmType(lhs) += rhs);
            } else {
                return QasmType<unsigned int, RhsN, true>(QasmType(lhs) += rhs);
            }
        } else {
            return QasmType(lhs) += rhs;
        }
    }

    template <Integer IntT>
    constexpr friend QasmType operator+(const IntT& lhs, const QasmType& rhs) {
        return QasmType(lhs) += rhs;
    }

    constexpr friend double operator+(double lhs, const QasmType& rhs) {
        return lhs += static_cast<T>(rhs);
    }

    template <typename RhsT, unsigned int RhsN, bool isRhsAngle>
    constexpr friend QasmType operator-(
        const QasmType& lhs,
        const QasmType<RhsT, RhsN, isRhsAngle>& rhs) {
        if constexpr (isRhsAngle) {
            if constexpr (N > RhsN) {
                return QasmType<unsigned int, N, true>(QasmType(lhs) -= rhs);
            } else {
                return QasmType<unsigned int, RhsN, true>(QasmType(lhs) -= rhs);
            }
        } else {
            return QasmType(lhs) -= rhs;
        }
    }

    template <Integer IntT>
    constexpr friend QasmType operator-(const IntT& lhs, const QasmType& rhs) {
        return QasmType(lhs) -= rhs;
    }

    constexpr friend double operator-(double lhs, const QasmType& rhs) {
        return lhs -= static_cast<T>(rhs);
    }

    template <typename RhsT, unsigned int RhsN, bool isRhsAngle>
    constexpr friend auto operator*(
        const QasmType& lhs,
        const QasmType<RhsT, RhsN, isRhsAngle>& rhs) {
        if constexpr (isRhsAngle) {
            if constexpr (N > RhsN) {
                return QasmType<unsigned int, N, true>(QasmType(lhs) *= rhs);
            } else {
                return QasmType<unsigned int, RhsN, true>(QasmType(lhs) *= rhs);
            }
        } else {
            return QasmType(lhs) *= rhs;
        }
    }

    template <Integer IntT>
    constexpr friend QasmType operator*(const IntT& lhs, const QasmType& rhs) {
        return QasmType(lhs) *= rhs;
    }

    constexpr friend double operator*(double lhs, const QasmType& rhs) {
        return lhs *= static_cast<T>(rhs);
    }

    template <typename RhsT, unsigned int RhsN, bool isRhsAngle>
    constexpr friend QasmType operator/(
        const QasmType& lhs,
        const QasmType<RhsT, RhsN, isRhsAngle>& rhs) {
        if constexpr (isRhsAngle) {
            if constexpr (N > RhsN) {
                return QasmType<unsigned int, N, true>(QasmType(lhs) /= rhs);
            } else {
                return QasmType<unsigned int, RhsN, true>(QasmType(lhs) /= rhs);
            }
        } else {
            return QasmType(lhs) /= rhs;
        }
    }

    template <Integer IntT>
    constexpr friend QasmType operator/(const IntT& lhs, const QasmType& rhs) {
        return QasmType(lhs) /= rhs;
    }

    constexpr friend double operator/(double lhs, const QasmType& rhs) {
        return lhs /= static_cast<double>(static_cast<T>(rhs));
    }

    template <typename OtherT>
    constexpr QasmType& operator+=(const OtherT& rhs) {
        auto lhsVal = static_cast<T>(*this);
        auto rhsVal = static_cast<OtherT>(rhs);
        auto result = lhsVal + rhsVal;
        writeBits(result);
        return *this;
    }

    template <typename OtherT>
    constexpr QasmType& operator-=(const OtherT& rhs) {
        auto lhsVal = static_cast<T>(*this);
        auto rhsVal = static_cast<OtherT>(rhs);
        auto result = lhsVal - rhsVal;
        writeBits(result);
        return *this;
    }

    template <typename OtherT>
    constexpr QasmType& operator*=(const OtherT& rhs) {
        auto lhsVal = static_cast<T>(*this);
        auto rhsVal = static_cast<OtherT>(rhs);
        auto result = lhsVal * rhsVal;
        writeBits(result);
        return *this;
    }

    template <typename OtherT>
    constexpr QasmType& operator/=(const OtherT& rhs) {
        auto lhsVal = static_cast<T>(*this);
        auto rhsVal = static_cast<OtherT>(rhs);
        auto result = lhsVal / rhsVal;
        writeBits(result);
        return *this;
    }

    constexpr QasmType& operator<<=(ssize shift) {
        if (shift >= N) {
            bits.fill(0);
            return *this;
        }

        for (ssize i = N; i > 0; --i) {
            ssize idx = i - 1;
            if (idx >= shift) {
                bits[idx] = bits[idx - shift];
            } else {
                bits[idx] = false;
            }
        }
        return *this;
    }

    constexpr QasmType& operator>>=(ssize shift) {
        if (shift >= N) {
            bits.fill(0);
            return *this;
        }

        for (ssize i = 0; i < N; ++i) {
            if (i + shift < N) {
                bits[i] = bits[i + shift];
            } else {
                bits[i] = false;
            }
        }

        return *this;
    }

    constexpr QasmType& operator^=(const QasmType<T, N>& rhs) {
        for (ssize i = 0; i < N; ++i) {
            bits[i] = bits[i] != rhs.bits[i];
        }
        return *this;
    }

    constexpr QasmType& operator|=(const QasmType<T, N>& rhs) {
        for (ssize i = 0; i < N; ++i) {
            bits[i] = bits[i] || rhs.bits[i];
        }

        return *this;
    }

    constexpr QasmType& operator&=(const QasmType<T, N>& rhs) {
        for (ssize i = 0; i < N; ++i) {
            bits[i] = bits[i] && rhs.bits[i];
        }

        return *this;
    }

    constexpr QasmType& operator~() {
        for (ssize i = 0; i < N; ++i) {
            bits[i] = !bits[i];
        }

        return *this;
    }

    constexpr QasmType operator-() {
        auto val = static_cast<T>(*this);
        auto result = -val;
        writeBits(result);
        return *this;
    }

    constexpr friend QasmType operator<<(const QasmType& lhs, int rhs) {
        return QasmType(lhs) <<= rhs;
    }

    template <Integer IntT>
    constexpr friend QasmType operator<<(const IntT& lhs, const QasmType& rhs) {
        return QasmType(lhs) <<= rhs;
    }

    constexpr friend QasmType operator>>(const QasmType& lhs, int rhs) {
        return QasmType(lhs) >>= rhs;
    }

    template <Integer IntT>
    constexpr friend QasmType operator>>(const IntT& lhs, const QasmType& rhs) {
        return QasmType(lhs) >>= rhs;
    }

    template <typename RhsT, unsigned int RhsN>
    constexpr friend QasmType operator>>(const QasmType& lhs,
                                         const QasmType<RhsT, RhsN>& rhs) {
        return QasmType(lhs) >>= rhs;
    }

    template <typename RhsT, unsigned int RhsN>
    constexpr friend QasmType operator^(const QasmType& lhs,
                                        const QasmType<RhsT, RhsN>& rhs) {
        return QasmType(lhs) ^= rhs;
    }

    template <Integer IntT>
    constexpr friend QasmType operator^(const IntT& lhs, const QasmType& rhs) {
        return QasmType(lhs) ^= rhs;
    }

    template <typename RhsT, unsigned int RhsN>
    constexpr friend QasmType operator&(const QasmType& lhs,
                                        const QasmType<RhsT, RhsN>& rhs) {
        return QasmType(lhs) &= rhs;
    }

    template <Integer IntT>
    constexpr friend QasmType operator&(const IntT& lhs, const QasmType& rhs) {
        return QasmType(lhs) &= rhs;
    }

    template <typename RhsT, unsigned int RhsN>
    constexpr friend QasmType operator|(const QasmType& lhs,
                                        const QasmType<RhsT, RhsN>& rhs) {
        return QasmType(lhs) |= rhs;
    }

    template <Integer IntT>
    constexpr friend QasmType operator|(const IntT& lhs, const QasmType& rhs) {
        return QasmType(lhs) |= rhs;
    }
};

template <typename T, unsigned int N, bool isAngle>
std::string toStr(const QasmType<T, N, isAngle>& type) {
    std::string result{""};
    for (auto bit : type.bits | std::views::reverse) {
        if (bit == 0) {
            result += '0';
        } else {
            result += '1';
        }
    }
    std::cout << result << "\n";
    return result;
}

template <unsigned int N>
constexpr QasmType<unsigned int, N> rotl(QasmType<unsigned int, N> x, int s) {
    QasmType<unsigned int, N> result(x);

    std::vector<bool> store;

    for (ssize i = N - s; i < N; ++i) {
        store.emplace_back(result.bits[i]);
    }

    for (ssize i = N - s - 1; i >= 0; --i) {
        result.bits[i + s] = result.bits[i];
    }

    for (ssize i = 0; i < store.size(); ++i) {
        result.bits[i] = store[i];
    }

    return result;
}

template <unsigned int N>
constexpr QasmType<unsigned int, N> rotr(QasmType<unsigned int, N> x, int s) {
    QasmType<unsigned int, N> result(x);

    std::vector<bool> store(s);

    for (ssize i = 0; i < s; ++i) {
        store[i] = result.bits[i];
    }

    for (ssize i = s; i < N; ++i) {
        result.bits[i - s] = result.bits[i];
    }

    for (ssize i = 0; i < store.size(); ++i) {
        result.bits[N - s + i] = store[i];
    }

    return result;
}

template <typename T, unsigned int N>
constexpr ssize popcount(QasmType<T, N> x) {
    ssize result = 0;

    for (auto bit : x.bits) {
        if (bit == 1) {
            ++result;
        }
    }

    return result;
}

inline int getGlobalQubitIdx(const std::vector<int>& indices, int idx) {
    idx = idx < 0 ? indices.size() + idx : idx;
    return indices[idx];
}

inline int getGlobalQubitIdx(const QuregSlice& qureg, int idx) {
    auto start = qureg.start;
    if (idx < 0) {
        start = qureg.end;
    }
    const auto newIdx = start + idx * qureg.increment;
    if (newIdx >= qureg.end || newIdx < qureg.start) {
        throw std::runtime_error("index out of bounds.");
    }
    if (qureg.parentSlice) {
        return getGlobalQubitIdx(*qureg.parentSlice, newIdx);
    }
    return newIdx;
};

inline QuregSlice getSlice(QuregSlice& q,
                           int start,
                           int end,
                           int increment = 1) {
    return q.slice(start, end, increment);
}

template <typename T>
class SliceView {
   private:
    std::vector<T>& parent_;
    ssize start_;
    ssize size_;
    ssize increment_;

    ssize handleIndex(ssize idx, ssize length) const {
        if (idx < 0) {
            return length + idx;
        }
        return idx;
    }

   public:
    SliceView(std::vector<T>& v, ssize start, ssize end, ssize increment)
        : parent_(v), increment_(increment) {
        ssize len = static_cast<ssize>(v.size());

        // handle potentially negative indices (-1 -> len - 1)
        start_ = handleIndex(start, len);
        ssize trueEnd = handleIndex(end, len);

        // handle boundaries and negative stride
        if (increment_ > 0) {
            if (start_ < 0) {
                start_ = 0;
            }
            // end is inclusive so increment
            ++trueEnd;
            if (trueEnd > len) {
                trueEnd = len;
            }

            if (start_ < trueEnd) {
                // end is inclusive so ceil it
                size_ = static_cast<ssize>(
                    (static_cast<double>((trueEnd - start_ + increment_ - 1)) /
                     static_cast<double>(increment_)));
            } else {
                size_ = 0;
            }
        } else {
            if (start_ >= len) {
                start_ = len - 1;
            }

            // end is inclusive so decrement
            --trueEnd;
            if (trueEnd < -1) {
                trueEnd = -1;
            }

            if (start_ > trueEnd) {
                size_ = static_cast<ssize>(
                    (static_cast<double>((start_ - trueEnd - increment_ - 1)) /
                     static_cast<double>(-increment_)));
            } else {
                size_ = 0;
            }
        }
    }

    SliceView(const SliceView& other, ssize start, ssize end, ssize increment)
        : parent_(other.parent_) {
        // end is inclusive

        ssize parentLen = static_cast<ssize>(other.size());

        ssize childStart = handleIndex(start, parentLen);
        ssize childEnd = handleIndex(end, parentLen);

        // create temp to make idx calculation easier
        SliceView<T> temp(other.parent_, childStart, childEnd, increment);

        // set values to calculate idx from root
        this->start_ = other.start_ + (temp.start_ * other.increment_);
        this->increment_ = other.increment_ * temp.increment_;
        this->size_ = temp.size_;
    }

    size_t size() const {
        return static_cast<size_t>(size_);
    }

    T& operator[](size_t index) {
        return parent_[start_ + (static_cast<ssize>(index) * increment_)];
    }

    const T& operator[](size_t index) const {
        return parent_[start_ + (static_cast<ssize>(index) * increment_)];
    }
};

template <typename T>
SliceView<T> getSlice(std::vector<T>& v,
                      int start,
                      int end,
                      int increment = 1) {
    return {v, start, end, increment};
}

template <typename T>
SliceView<T> getSlice(SliceView<T>& slice,
                      int start,
                      int end,
                      int increment = 1) {
    return {slice, start, end, increment};
}

template <typename T, unsigned int N>
constexpr std::vector<bool> getSlice(const QasmType<T, N>& v,
                                     int start,
                                     int end,
                                     int increment = 1) {
    start = start < 0 ? v.bits.size() + start : start;
    end = end < 0 ? v.bits.size() + end : end;

    // NOTE: end is inclusive
    std::vector<bool> result;
    result.reserve(static_cast<ssize>((start + end) / 2));
    if (end < start) {
        if (increment > 0) {
            increment = -increment;
        }
        --end;
        for (int i = start; i > end; i += increment) {
            result.push_back(v[i]);
        }
    } else {
        ++end;
        for (int i = start; i < end; i += increment) {
            result.push_back(v[i]);
        }
    }

    return result;
}

template <typename T>
std::vector<T>& setSlice(std::vector<T>& to,
                         std::vector<T>& from,
                         int start,
                         int end,
                         int increment) {
    start = start < 0 ? to.size() + start : start;
    end = end < 0 ? to.size() + end : end;
    // NOTE: end is inclusive
    if (end < start) {
        if (increment > 0) {
            increment = -increment;
        }
        --end;
        int j = 0;
        for (int i = start; i > end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    } else {
        ++end;
        int j = 0;
        for (int i = start; i < end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    }
    return to;
}

template <typename T>
std::vector<T>& setSlice(std::vector<T>& to,
                         SliceView<T>& from,
                         int start,
                         int end,
                         int increment) {
    start = start < 0 ? to.size() + start : start;
    end = end < 0 ? to.size() + end : end;
    // NOTE: end is inclusive
    if (end < start) {
        if (increment > 0) {
            increment = -increment;
        }
        --end;
        int j = 0;
        for (int i = start; i > end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    } else {
        ++end;
        int j = 0;
        for (int i = start; i < end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    }
    return to;
}

template <typename T>
SliceView<T>& setSlice(SliceView<T>& to,
                       std::vector<T>& from,
                       int start,
                       int end,
                       int increment) {
    start = start < 0 ? to.size() + start : start;
    end = end < 0 ? to.size() + end : end;
    // NOTE: end is inclusive
    if (end < start) {
        if (increment > 0) {
            increment = -increment;
        }
        --end;
        int j = 0;
        for (int i = start; i > end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    } else {
        ++end;
        int j = 0;
        for (int i = start; i < end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    }
    return to;
}

template <typename T>
SliceView<T>& setSlice(SliceView<T>& to,
                       SliceView<T>& from,
                       int start,
                       int end,
                       int increment) {
    start = start < 0 ? to.size() + start : start;
    end = end < 0 ? to.size() + end : end;
    // NOTE: end is inclusive
    if (end < start) {
        if (increment > 0) {
            increment = -increment;
        }
        --end;
        int j = 0;
        for (int i = start; i > end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    } else {
        ++end;
        int j = 0;
        for (int i = start; i < end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    }
    return to;
}

template <typename T>
std::vector<T>& setSlice(std::vector<T>& to,
                         std::vector<T>&& from,
                         int start,
                         int end,
                         int increment) {
    start = start < 0 ? to.size() + start : start;
    end = end < 0 ? to.size() + end : end;

    // NOTE: end is inclusive
    if (end < start) {
        if (increment > 0) {
            increment = -increment;
        }
        --end;
        int j = 0;
        for (int i = start; i > end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    } else {
        ++end;
        int j = 0;
        for (int i = start; i < end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    }
    return to;
}

template <typename T>
std::vector<T>& setSlice(std::vector<T>& to,
                         SliceView<T>&& from,
                         int start,
                         int end,
                         int increment) {
    start = start < 0 ? to.size() + start : start;
    end = end < 0 ? to.size() + end : end;

    // NOTE: end is inclusive
    if (end < start) {
        if (increment > 0) {
            increment = -increment;
        }
        --end;
        int j = 0;
        for (int i = start; i > end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    } else {
        ++end;
        int j = 0;
        for (int i = start; i < end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    }
    return to;
}

template <typename T>
SliceView<T>& setSlice(SliceView<T>& to,
                       std::vector<T>&& from,
                       int start,
                       int end,
                       int increment) {
    start = start < 0 ? to.size() + start : start;
    end = end < 0 ? to.size() + end : end;
    // NOTE: end is inclusive
    if (end < start) {
        if (increment > 0) {
            increment = -increment;
        }
        --end;
        int j = 0;
        for (int i = start; i > end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    } else {
        ++end;
        int j = 0;
        for (int i = start; i < end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    }
    return to;
}

template <typename T>
SliceView<T>& setSlice(SliceView<T>& to,
                       SliceView<T>&& from,
                       int start,
                       int end,
                       int increment) {
    start = start < 0 ? to.size() + start : start;
    end = end < 0 ? to.size() + end : end;
    // NOTE: end is inclusive
    if (end < start) {
        if (increment > 0) {
            increment = -increment;
        }
        --end;
        int j = 0;
        for (int i = start; i > end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    } else {
        ++end;
        int j = 0;
        for (int i = start; i < end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    }
    return to;
}

template <typename T>
std::vector<T> setSlice(std::vector<T>&& v, std::vector<T>&& w) {
    std::vector<T> result(v.size());
    for (int i = 0; i < v.size(); ++i) {
        result[i] = w[i];
    }
    return result;
}

template <typename T>
SliceView<T> setSlice(SliceView<T>&& v, SliceView<T>&& w) {
    SliceView<T> result(v.size());
    for (int i = 0; i < v.size(); ++i) {
        result[i] = w[i];
    }
    return result;
}

template <typename T, unsigned int N>
QasmType<T, N> setSlice(QasmType<T, N>& to,
                        std::vector<T>&& from,
                        int start,
                        int end,
                        int increment) {
    // NOTE: end is inclusive
    start = start < 0 ? to.bits.size() + start : start;
    end = end < 0 ? to.bits.size() + end : end;
    if (end < start) {
        if (increment > 0) {
            increment = -increment;
        }
        --end;
        int j = 0;
        for (int i = start; i > end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    } else {
        ++end;
        int j = 0;
        for (int i = start; i < end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    }
    return to;
}

template <typename T, unsigned int N>
QasmType<T, N> setSlice(QasmType<T, N>& to,
                        SliceView<T>&& from,
                        int start,
                        int end,
                        int increment) {
    // NOTE: end is inclusive
    start = start < 0 ? to.bits.size() + start : start;
    end = end < 0 ? to.bits.size() + end : end;
    if (end < start) {
        if (increment > 0) {
            increment = -increment;
        }
        --end;
        int j = 0;
        for (int i = start; i > end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    } else {
        ++end;
        int j = 0;
        for (int i = start; i < end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    }
    return to;
}

template <typename T, unsigned int N>
QasmType<T, N> setSlice(QasmType<T, N>& to,
                        QasmType<T, N>&& from,
                        int start,
                        int end,
                        int increment) {
    // NOTE: end is inclusive

    start = start < 0 ? to.bits.size() + start : start;
    end = end < 0 ? to.bits.size() + end : end;
    if (end < start) {
        if (increment > 0) {
            increment = -increment;
        }
        --end;
        int j = 0;
        for (int i = start; i > end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    } else {
        ++end;
        int j = 0;
        for (int i = start; i < end; i += increment) {
            to[i] = from[j];
            ++j;
        }
    }
    return to;
}

template <typename T, unsigned int N>
QasmType<T, N> setSlice(QasmType<T, N>& to,
                        std::string&& from,
                        int start,
                        int end,
                        int increment) {
    // NOTE: end is inclusive

    std::reverse(from.begin(), from.end());
    start = start < 0 ? to.bits.size() + start : start;
    end = end < 0 ? to.bits.size() + end : end;
    if (end < start) {
        if (increment > 0) {
            increment = -increment;
        }
        --end;
        int j = 0;
        for (int i = start; i > end; i += increment) {
            to[i] = from[j] == '0' ? 0 : 1;
            ++j;
        }
    } else {
        ++end;
        int j = 0;
        for (int i = start; i < end; i += increment) {
            to[i] = from[j] == '0' ? 0 : 1;
            ++j;
        }
    }
    return to;
}
}    // namespace qasc
