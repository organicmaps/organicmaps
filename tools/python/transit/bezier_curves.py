"""Copyright (c) 2015, Emilia Petrisor
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."""

import numpy as np


def knot_interval(i_pts, alpha=0.5, closed=False):
    if len(i_pts) < 4:
        raise ValueError('CR-curves need at least 4 interpolatory points')
        # i_pts is the list of interpolatory points P[0], P[1], ... P[n]
    if closed:
        i_pts += [i_pts[0], i_pts[1], i_pts[2]]
    i_pts = np.array(i_pts)
    dist = np.linalg.norm(i_pts[1:, :] - i_pts[:-1, :], axis=1)
    return dist ** alpha


def ctrl_bezier(P, d):
    # Associate to 4 consecutive interpolatory points and the corresponding three d-values,
    # the Bezier control points
    if len(P) != len(d) + 1 != 4:
        raise ValueError('The list of points and knot intervals have inappropriate len ')
    P = np.array(P)
    bz = [0] * 4
    bz[0] = P[1]
    bz[1] = (d[0] ** 2 * P[2] - d[1] ** 2 * P[0] + (2 * d[0] ** 2 + 3 * d[0] * d[1] + d[1] ** 2) * P[1]) / (
    3 * d[0] * (d[0] + d[1]))
    bz[2] = (d[2] ** 2 * P[1] - d[1] ** 2 * P[3] + (2 * d[2] ** 2 + 3 * d[2] * d[1] + d[1] ** 2) * P[2]) / (
    3 * d[2] * (d[1] + d[2]))
    bz[3] = P[2]
    return bz


def Bezier_curve(bz, nr=100):
    # implements the de Casteljau algorithm to compute nr points on a Bezier curve
    t = np.linspace(0, 1, nr)
    N = len(bz)
    points = []  # the list of points to be computed on the Bezier curve
    for i in range(nr):  # for each parameter t[i] evaluate a point on the Bezier curve
        # via De Casteljau algorithm
        aa = np.copy(bz)
        for r in range(1, N):
            aa[:N - r, :] = (1 - t[i]) * aa[:N - r, :] + t[i] * aa[1:N - r + 1, :]  # convex combination
        points.append(aa[0, :])
    return points


def Catmull_Rom(i_pts, alpha=0.5, closed=False):
    # returns the list of points computed on the interpolating CR curve
    # i_pts the list of interpolatory points P[0], P[1], ...P[n]
    curve_pts = []  # the list of all points to be computed on the CR curve
    d = knot_interval(i_pts, alpha=alpha, closed=closed)
    for k in range(len(i_pts) - 3):
        cb = ctrl_bezier(i_pts[k:k + 4], d[k:k + 3])
        curve_pts.extend(Bezier_curve(cb, nr=100))

    return np.array(curve_pts)


def segment_to_Catmull_Rom_curve(p1, s1, s2, p2, nr=100, alpha=0.5):
    i_pts = [p1, s1, s2, p2]
    # returns the list of points computed on the interpolating CR curve
    # i_pts the list of interpolatory points P[0], P[1], ...P[n]
    curve_pts = []  # the list of all points to be computed on the CR curve
    d = knot_interval(i_pts, alpha=alpha, closed=False)
    cb = ctrl_bezier(i_pts, d)
    curve_pts.extend(Bezier_curve(cb, nr=nr))
    return curve_pts
