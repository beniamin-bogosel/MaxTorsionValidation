# Exact symbolic regression checks

These scripts use exact SymPy arithmetic.  They do not validate identities by
floating-point sampling: a check succeeds only when every relevant rational or
polynomial expression reduces identically to zero.

- `verify_hessian_reduction.py` checks the two-dimensional reduction of
  Laurain's local torsion-Hessian integrand to equation `(HJ)`, the planar skew
  identity used in that reduction, and block-transpose reciprocity.
- `verify_pullback_coefficients.py` differentiates
  `det(F) F^{-1} F^{-T}` for `F=I+sQ+tR` and checks the first and mixed
  coefficients against the component formulas in `second_lifting_cert.c`.
- `equilateral_sector_integrals.py` checks the exact triangular-sector values
  already used for the equilateral case.
- `verify_residual_identities.py` checks the second-variation Galerkin error
  identity and both corrected-center identities against a differentiated
  SPD energy, with independent symbolic loads, coefficients, and candidates.

Run all checks from the repository root with

```sh
make -C MaxTorsionValidation/flint test-symbolic
```
