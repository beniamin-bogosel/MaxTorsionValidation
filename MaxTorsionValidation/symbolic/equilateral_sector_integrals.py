#!/usr/bin/env python3
"""Exact check of the n=3 sector formulas in the theoretical manuscript."""

import sympy as sp


x, y, lam, mu = sp.symbols("x y lam mu", real=True)
sqrt3 = sp.sqrt(3)

# Circumradius-one equilateral triangle with vertices
# (1,0), (-1/2,sqrt(3)/2), and (-1/2,-sqrt(3)/2).
product = (x + sp.Rational(1, 2)) * ((1 - x) ** 2 - 3 * y**2)
w = sp.expand(product / 6)
assert sp.simplify(-sp.diff(w, x, 2) - sp.diff(w, y, 2) - 1) == 0

# Affine coordinates on T_0=conv{0,(1,0),(-1/2,sqrt(3)/2)}:
# (x,y)=lam*(1,0)+mu*(-1/2,sqrt(3)/2).
x_sector = lam - mu / 2
y_sector = sqrt3 * mu / 2
jacobian = sqrt3 / 2


def sector_integral(expression):
    pulled_back = sp.expand(
        expression.subs({x: x_sector, y: y_sector}) * jacobian
    )
    return sp.simplify(
        sp.integrate(
            sp.integrate(pulled_back, (lam, 0, 1 - mu)),
            (mu, 0, 1),
        )
    )


wx, wy = sp.diff(w, x), sp.diff(w, y)
X = sector_integral(wx**2)
Y = sector_integral(wy**2)
Z = sector_integral(wx * wy)
W0 = sector_integral(w)
J = sp.simplify(3 * W0 / 2)

expected = (
    sqrt3 / 384,
    13 * sqrt3 / 1920,
    sp.Rational(1, 160),
    3 * sqrt3 / 320,
    9 * sqrt3 / 640,
)
assert all(sp.simplify(a - b) == 0 for a, b in zip((X, Y, Z, W0, J), expected))

print(f"w = {w}")
print(f"X = {X}")
print(f"Y = {Y}")
print(f"Z = {Z}")
print(f"integral_T0(w) = {W0}")
print(f"J = {J}")
