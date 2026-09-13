#!/usr/bin/env python3
"""Exact checks of both second-variation identities with independent data.

Differentiate the energy of a parameter-dependent SPD system, then compare
with the residual formulas. All first/mixed coefficients, loads, and rounded
candidates are independent symbols; the Galerkin space is a proper subspace.
"""
import sympy as s


def scalar(value):
    return value[0]


def check(name, value):
    if s.expand(value) != 0:
        raise AssertionError(name)
    print("verified " + name)


def symmetric(name):
    a, b, c = s.symbols(name + "0:3")
    return s.Matrix([[a, b], [b, c]])


K = s.Matrix([[3, 1], [1, 2]])
Kq, Kr, Kqr = (symmetric(name) for name in ("Q", "R", "H"))
f, fq, fr, fqr = (s.Matrix(s.symbols(name + "0:2"))
                  for name in ("f", "fq", "fr", "fqr"))
u = K.inv() * f
uq = K.inv() * (fq - Kq * u)
ur = K.inv() * (fr - Kr * u)
uqr = K.inv() * (fqr - Kq * ur - Kr * uq - Kqr * u)
# Direct mixed derivative of J = f^T u / 2 by the product rule.
Jqr = scalar(fqr.T*u + fq.T*ur + fr.T*uq + f.T*uqr) / 2
uh = s.Matrix([f[0]/K[0, 0], 0])
uhq = s.Matrix([(fq[0] - (Kq*uh)[0])/K[0, 0], 0])
uhr = s.Matrix([(fr[0] - (Kr*uh)[0])/K[0, 0], 0])
uhqr = s.Matrix([(fqr[0] - (Kq*uhr + Kr*uhq + Kqr*uh)[0])/K[0, 0], 0])
Jhqr = scalar(fqr.T*uh + fq.T*uhr + fr.T*uhq + f.T*uhqr) / 2
e, eq, er = u-uh, uq-uhq, ur-uhr
lifting_load = fqr - Kq*uhr - Kr*uhq - Kqr*uh
check("second-variation Galerkin residual identity",
      Jqr-Jhqr-scalar(eq.T*K*er-e.T*Kqr*e/2+lifting_load.T*e))

v, vq, vr, vz = (s.Matrix(s.symbols(name + "0:2"))
                 for name in ("v", "vq", "vr", "vz"))
rho0 = f-K*v
rhor = fr-Kr*v-K*vr
rhoz = fqr-Kq*vr-Kr*vq-Kqr*v-K*vz
center = scalar(fqr.T*v + fq.T*vr - v.T*Kqr*v/2 - v.T*Kq*vr
                + rhor.T*vq + rho0.T*vz)
d0, dq, dr = u-v, uq-vq, ur-vr
check("residual-corrected Hessian center identity",
      Jqr-center-scalar(rhoz.T*d0-d0.T*Kqr*d0/2+dq.T*K*dr))
check("residual-corrected energy center identity",
      scalar(f.T*u)/2-scalar(f.T*v-v.T*K*v/2+d0.T*K*d0/2))
