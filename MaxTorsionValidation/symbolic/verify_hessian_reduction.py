#!/usr/bin/env python3
"""Verify the planar reduction of Laurain's torsion Hessian integrand.

The check is exact: every entry of the difference is reduced to a zero
polynomial by SymPy.  Vectors ``p``, ``q``, and ``g`` stand for
``grad(phi_i)``, ``grad(phi_j)``, and ``grad(w)``, respectively.  The common
Gram term ``DU_i DU_j^T`` is absent because it is unchanged by the reduction.
"""

import sympy as sp


def outer(left: sp.Matrix, right: sp.Matrix) -> sp.Matrix:
    """Return the tensor product left (x) right."""

    return left * right.T


def require_zero_matrix(name: str, matrix: sp.Matrix) -> None:
    """Fail with the unreduced entries if ``matrix`` is not identically zero."""

    reduced = matrix.applyfunc(lambda entry: sp.factor(sp.expand(entry)))
    failures = [
        (row, column, reduced[row, column])
        for row in range(reduced.rows)
        for column in range(reduced.cols)
        if reduced[row, column] != 0
    ]
    if failures:
        details = ", ".join(
            f"({row},{column}): {entry}"
            for row, column, entry in failures
        )
        raise SystemExit(f"{name} failed: {details}")


p1, p2, q1, q2, g1, g2, w = sp.symbols(
    "p1 p2 q1 q2 g1 g2 w", real=True
)
p = sp.Matrix([p1, p2])
q = sp.Matrix([q1, q2])
g = sp.Matrix([g1, g2])
identity = sp.eye(2)
g_squared = g.dot(g)

# Laurain's unreduced local integrand (the terms following DU_i DU_j^T).
# Here 2 p odot q = p (x) q + q (x) p.
s1 = (-g_squared / 2 + w) * identity + outer(g, g)
full_local_integrand = (
    outer(p, s1 * q)
    + outer(s1 * p, q)
    + (g_squared / 2 - w) * (outer(p, q) + outer(q, p))
    - p.dot(g) * outer(q, g)
    - q.dot(g) * outer(g, p)
    - p.dot(q) * outer(g, g)
)

# The compact formula used in equation (HJ).
reduced_local_integrand = (
    (g_squared / 2 + w) * (outer(p, q) - outer(q, p))
    - p.dot(q) * outer(g, g)
)

require_zero_matrix(
    "Laurain-to-(HJ) local-integrand reduction",
    full_local_integrand - reduced_local_integrand,
)

# This is the dimension-specific identity responsible for the reduction.
cross_terms = (
    q.dot(g) * outer(p, g)
    + p.dot(g) * outer(g, q)
    - p.dot(g) * outer(q, g)
    - q.dot(g) * outer(g, p)
)
require_zero_matrix(
    "two-dimensional skew identity",
    cross_terms - g_squared * (outer(p, q) - outer(q, p)),
)

# A block Hessian must obey T_ji = T_ij^T.
reduced_with_indices_swapped = (
    (g_squared / 2 + w) * (outer(q, p) - outer(p, q))
    - p.dot(q) * outer(g, g)
)
require_zero_matrix(
    "block-transpose reciprocity",
    reduced_with_indices_swapped - reduced_local_integrand.T,
)

print("verified exact Laurain-to-(HJ) reduction in dimension two")
print("verified exact two-dimensional skew identity")
print("verified T_ji = T_ij^T for the reduced local integrand")
