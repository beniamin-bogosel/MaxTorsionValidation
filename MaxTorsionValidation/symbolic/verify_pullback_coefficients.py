#!/usr/bin/env python3
"""Verify the pullback coefficients used by ``second_lifting_cert.c``.

For ``F(s,t) = I + s Q + t R``, the certifier uses derivatives at the
origin of

    A(s,t) = det(F(s,t)) F(s,t)^(-1) F(s,t)^(-T).

This script differentiates that defining rational matrix exactly and compares
the result with both the coordinate-free formulas and the component formulas
implemented in C.  No floating-point substitutions are used.
"""

import sympy as sp


def reduce_entry(entry: sp.Expr) -> sp.Expr:
    """Put a rational expression in a canonical exact form."""

    return sp.factor(sp.cancel(entry))


def require_zero(name: str, expression: sp.Expr) -> None:
    reduced = reduce_entry(expression)
    if reduced != 0:
        raise SystemExit(f"{name} failed: {reduced}")


def require_zero_matrix(name: str, matrix: sp.Matrix) -> None:
    reduced = matrix.applyfunc(reduce_entry)
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


s, t = sp.symbols("s t", real=True)
a, b, c, d, e, f, g, h = sp.symbols(
    "a b c d e f g h", real=True
)
q = sp.Matrix([[a, b], [c, d]])
r = sp.Matrix([[e, f], [g, h]])
identity = sp.eye(2)
deformation = identity + s * q + t * r
determinant = deformation.det()
inverse = deformation.inv()
pullback = determinant * inverse * inverse.T
origin = {s: 0, t: 0}

det_q_from_definition = sp.diff(determinant, s).subs(origin)
det_r_from_definition = sp.diff(determinant, t).subs(origin)
det_qr_from_definition = sp.diff(determinant, s, t).subs(origin)
a_q_from_definition = pullback.diff(s).subs(origin)
a_r_from_definition = pullback.diff(t).subs(origin)
a_qr_from_definition = pullback.diff(s, t).subs(origin)

trace_q = sp.trace(q)
trace_r = sp.trace(r)
det_qr = trace_q * trace_r - sp.trace(q * r)
a_q = trace_q * identity - q - q.T
a_r = trace_r * identity - r - r.T

# Mixed derivative of F^{-1}F^{-T} at the identity.
inverse_metric_qr = (
    q * r
    + r * q
    + (q * r + r * q).T
    + q * r.T
    + r * q.T
)
a_qr_invariant = (
    det_qr * identity
    - trace_q * (r + r.T)
    - trace_r * (q + q.T)
    + inverse_metric_qr
)

require_zero("first determinant derivative in q", det_q_from_definition - trace_q)
require_zero("first determinant derivative in r", det_r_from_definition - trace_r)
require_zero(
    "mixed determinant derivative",
    det_qr_from_definition - det_qr,
)
require_zero_matrix("A_q invariant formula", a_q_from_definition - a_q)
require_zero_matrix("A_r invariant formula", a_r_from_definition - a_r)
require_zero_matrix(
    "A_qr invariant formula",
    a_qr_from_definition - a_qr_invariant,
)

# Literal transcription of coefficients() in second_lifting_cert.c.  Only
# (0,0), (0,1), and (1,1) are stored there because A is symmetric.
d_qr_code = (
    trace_q * trace_r - a * e - b * g - c * f - d * h
)
s11_code = 6 * a * e + 2 * (b * g + f * c + b * f)
s22_code = 6 * d * h + 2 * (c * f + b * g + c * g)
s12_code = (
    a * f
    + 2 * b * h
    + e * b
    + 2 * f * d
    + 2 * c * e
    + d * g
    + 2 * g * a
    + h * c
)
a_q_code = sp.Matrix(
    [
        [trace_q - 2 * a, -(b + c)],
        [-(b + c), trace_q - 2 * d],
    ]
)
a_r_code = sp.Matrix(
    [
        [trace_r - 2 * e, -(f + g)],
        [-(f + g), trace_r - 2 * h],
    ]
)
a_qr_code = sp.Matrix(
    [
        [
            d_qr_code - 2 * trace_q * e - 2 * trace_r * a + s11_code,
            -trace_q * (f + g) - trace_r * (b + c) + s12_code,
        ],
        [
            -trace_q * (f + g) - trace_r * (b + c) + s12_code,
            d_qr_code - 2 * trace_q * h - 2 * trace_r * d + s22_code,
        ],
    ]
)

require_zero("C determinant formula", d_qr_code - det_qr_from_definition)
require_zero_matrix("C A_q component formulas", a_q_code - a_q_from_definition)
require_zero_matrix("C A_r component formulas", a_r_code - a_r_from_definition)
require_zero_matrix(
    "C A_qr component formulas",
    a_qr_code - a_qr_from_definition,
)
require_zero_matrix(
    "mixed pullback symmetry",
    a_qr_from_definition - a_qr_from_definition.T,
)

print("verified d_q = tr(Q), d_r = tr(R), and d_qr = tr(Q)tr(R)-tr(QR)")
print("verified exact A_q and A_r formulas from det(F) F^(-1) F^(-T)")
print("verified exact invariant and C-component formulas for A_qr")
print("verified symmetry of the mixed pullback coefficient")
