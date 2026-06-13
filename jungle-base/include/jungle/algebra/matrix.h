// Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <new>
#include <ranges>
#include <type_traits>

#include "jungle/algebra/scalar.h"
#include "jungle/types/int.h"

namespace jungle::algebra {

template<scalar_type T, usize...>
class matrix;

template<scalar_type T, usize Line, usize Row>
class matrix<T, Line, Row> {
public:
    using value_type = T;

    constexpr matrix() = default;

    constexpr matrix(const matrix &) = default;
    constexpr matrix &operator=(const matrix &) = default;
    constexpr matrix(matrix &&) = default;
    constexpr matrix &operator=(matrix &&) = default;

    constexpr matrix(std::array<std::array<T, Row>, Line> data) {
        template for (constexpr auto i : std::views::iota(0u, Line)) {
            template for (constexpr auto j : std::views::iota(0u, Row)) { (*this)[i, j] = data[i][j]; }
        }
    }

    constexpr T &operator[](usize x, usize y) pre(x < Line && y < Row) { return m_data[x * Row + y]; }

    constexpr const T &operator[](usize x, usize y) const pre(x < Line && y < Row) {
        return m_data[x * Row + y];
    }

    consteval static matrix zero() {
        matrix result{};
        for (auto &d : result.m_data) {
            d = scalar_const<T>::zero;
        }
        return result;
    }

    matrix add(const matrix &rhs) const {
        matrix result{};
        template for (constexpr auto i : std::views::iota(0u, Line)) {
            template for (constexpr auto j : std::views::iota(0u, Row)) {
                result[i, j] = (*this)[i, j] + rhs[i, j];
            }
        }
        return result;
    }

    matrix subtract(const matrix &rhs) const {
        matrix result{};
        template for (constexpr auto i : std::views::iota(0u, Line)) {
            template for (constexpr auto j : std::views::iota(0u, Row)) {
                result[i, j] = (*this)[i, j] - rhs[i, j];
            }
        }
        return result;
    }

    matrix multiply(const T &scalar) const {
        matrix result{};
        template for (constexpr auto i : std::views::iota(0u, Line)) {
            template for (constexpr auto j : std::views::iota(0u, Row)) {
                result[i, j] = (*this)[i, j] * scalar;
            }
        }
        return result;
    }

    template<usize RRow>
    matrix<T, Line, RRow> multiply(const matrix<T, Row, RRow> &rhs) const {
        matrix<T, Line, RRow> result{};
        template for (constexpr auto i : std::views::iota(0u, Line)) {
            template for (constexpr auto j : std::views::iota(0u, RRow)) {
                T sum{scalar_const<T>::zero};
                template for (constexpr auto k : std::views::iota(0u, Row)) {
                    sum += (*this)[i, k] * rhs[k, j];
                }
                result[i, j] = sum;
            }
        }
        return result;
    }

    T dot(const matrix &rhs) const {
        T sum{scalar_const<T>::zero};
        template for (constexpr auto i : std::views::iota(0u, Line * Row)) {
            sum += m_data[i] * rhs.m_data[i];
        }
        return sum;
    }

    T norm() const {
        T sum{scalar_const<T>::zero};
        template for (constexpr auto i : std::views::iota(0u, Line * Row)) { sum += m_data[i] * m_data[i]; }
        return scalar_operator<T>::sqrt(sum);
    }

    matrix<T, Row, Line> transpose() const {
        matrix<T, Row, Line> result{};
        template for (constexpr auto i : std::views::iota(0u, Line)) {
            template for (constexpr auto j : std::views::iota(0u, Row)) { result[j, i] = (*this)[i, j]; }
        }
        return result;
    }

    template<usize RLine, usize RRow>
    matrix<T, Line * RLine, Row * RRow> kronecker_multiply(const matrix<T, RLine, RRow> &rhs) const {
        matrix<T, Line * RLine, Row * RRow> result{};
        template for (constexpr auto i : std::views::iota(0u, Line)) {
            template for (constexpr auto j : std::views::iota(0u, Row)) {
                template for (constexpr auto k : std::views::iota(0u, RLine)) {
                    template for (constexpr auto l : std::views::iota(0u, RRow)) {
                        result[i * RLine + k, j * RRow + l] = (*this)[i, j] * rhs[k, l];
                    }
                }
            }
        }
        return result;
    }

protected:
    std::array<T, Line * Row> m_data;
};

template<scalar_type T, usize N>
class matrix<T, N> : public matrix<T, N, N> {
public:
    using matrix<T, N, N>::matrix;

    static consteval matrix identity() {
        matrix result{};
        template for (constexpr auto i : std::views::iota(0u, N)) { result[i, i] = scalar_const<T>::one; }
        return result;
    }
};

template<scalar_type T, usize N>
class vector : public matrix<T, N, 1> {
public:
    constexpr vector() = default;

    constexpr vector(const matrix<T, N, 1> &m)
            : matrix<T, N, 1>{m} {}

    constexpr vector(std::array<T, N> data) {
        template for (constexpr auto i : std::views::iota(0u, N)) {
            matrix<T, N, 1>::operator[](i, 0) = data[i];
        }
    }

    constexpr T &operator[](usize i) pre(i < N) { return matrix<T, N, 1>::operator[](i, 0); }

    constexpr const T &operator[](usize i) const pre(i < N) { return matrix<T, N, 1>::operator[](i, 0); }

    vector normalize() const {
        return matrix<T, N, 1>::multiply(scalar_const<T>::one / matrix<T, N, 1>::norm());
    }

    vector cross(const vector &rhs) const
        requires(N == 3)
    {
        return vector{
            {(*this)[1] * rhs[2] - (*this)[2] * rhs[1], (*this)[2] * rhs[0] - (*this)[0] * rhs[2],
             (*this)[0] * rhs[1] - (*this)[1] * rhs[0]}};
    }
};

template<scalar_type T>
using matrix3 = matrix<T, 3>;

template<scalar_type T>
using matrix4 = matrix<T, 4>;

using matrix3f = matrix3<float>;
using matrix4f = matrix4<float>;

template<scalar_type T>
using vector2 = vector<T, 2>;

template<scalar_type T>
using vector3 = vector<T, 3>;

using vector2f = vector2<float>;
using vector3f = vector3<float>;

};  // namespace jungle::algebra
