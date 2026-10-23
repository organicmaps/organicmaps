//
// Created by Kai Zhao on 9/1/20.
//

#ifndef SZ3_INTERPOLATORS_HPP
#define SZ3_INTERPOLATORS_HPP

#include "SZ3/def.hpp"

namespace SZ3 {
template <class T>
T interp_linear(T a, T b) {
    return (a + b) / 2;
}

template <class T>
T interp_linear1(T a, T b) {
    return -0.5 * a + nofma(1.5 * b);
}

template <class T>
T interp_quad_1(T a, T b, T c) {
    return (nofma(3 * a) + nofma(6 * b) - c) / 8;
}

template <class T>
T interp_quad_2(T a, T b, T c) {
    return (-a + nofma(6 * b) + nofma(3 * c)) / 8;
}

template <class T>
T interp_quad_3(T a, T b, T c) {
    return (nofma(3 * a) - nofma(10 * b) + nofma(15 * c)) / 8;
}

template <class T>
T interp_cubic(T a, T b, T c, T d) {
    return (-a + nofma(9 * b) + nofma(9 * c) - d) / 16;
}

template <class T>
T interp_cubic_natural(T a, T b, T c, T d) {
   return nofma(0.575 * (b + c)) - nofma(0.075 * (a + d));
}

template<class T>
T lorenzo_1d(T a, T b) {
    return 2 * b - a;
}

template<class T>
T lorenzo_2d(T a, T b, T c) {
    return (b + c - a);
}

template<class T>
T lorenzo_3d(T a, T b, T c, T d, T e,T f,T g) {
    return (a - b - c + d - e + f + g);
}

    


template <class T>
T interp_cubic_front(T a, T b, T c, T d) {
    return (nofma(5 * a) + nofma(15 * b) - nofma(5 * c) + d) / 16;
}

template <class T>
T interp_cubic_front_2(T a, T b, T c, T d) {
    return (a + nofma(6 * b) - 4 * c + d) / 4;
}

template <class T>
T interp_cubic_back_1(T a, T b, T c, T d) {
    return (a - nofma(5 * b) + nofma(15 * c) + nofma(5 * d)) / 16;
}

template <class T>
T interp_cubic_back_2(T a, T b, T c, T d) {
    return (nofma(-5 * a) + nofma(21 * b) - nofma(35 * c) + nofma(35 * d)) / 16;
}

template <class T>
T interp_cubic2(T a, T b, T c, T d) {
    return (nofma(-3 * a) + nofma(23 * b) + nofma(23 * c) - nofma(3 * d)) / 40;
}

template <class T>
ALWAYS_INLINE T interp_akima(T a, T b, T c, T d) {
    T t0 = 2 * b - a - c;
    T t1 = 2 * c - b - d;
    T abt0 = fabs(t0);
    T abt1 = fabs(t1);
    if (fabs(abt0 + abt1) > 1e-9) {
        return (b + c) / 2 + (nofma(t0 * abt1) + nofma(t1 * abt0)) / 8 / (abt0 + abt1);
    } else {
        return (b + c) / 2;
    }
}

template <class T>
ALWAYS_INLINE T interp_pchip(T a, T b, T c, T d) {
    T pchip = (b + c) / 2;
    if ((b - a < 0) == (c - b < 0) && fabs(c - a) > 1e-9) {
        pchip += 0.25 * (b - a) * (c - b) / (c - a);
    }
    if ((c - b < 0) == (d - c < 0) && fabs(d - b) > 1e-9) {
        pchip -= 0.25 * (c - b) * (d - c) / (d - b);
    }
    return pchip;
}
}  // namespace SZ3
#endif  // SZ_INTERPOLATORS_HPP
