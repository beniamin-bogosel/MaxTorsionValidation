# Validity review, 9 September 2026

Publication note (13 September 2026): the review below records earlier work;
no numerical checks were rerun during the folder reorganization. Active
links now use `MaxTorsionValidation/`. Linked archive evidence and `Unused/`
files remain local or are supplied separately; see the
[archive guide](MaxTorsionValidation/results/README.md).

The reviewed mathematical and computational chain supports the reported
strict local maximality result for regular polygons through n = 25.
No defect was found that changes a published sign bound or the certified
range. The review corrected two overstatements about asymptotic convergence,
strengthened the executable and entry provenance of future runs, and
independently recertified one archived entry whose checksum differed.

## Mathematical corrections

- Section 6 now distinguishes the conditional quadratic convergence of the
  exact Galerkin Hessian from convergence of the computed enclosure radius.
  The latter also requires flux approximation and algebraic-error estimates.
- A finite sequence of mesh computations cannot establish an asymptotic
  lifting bound. The text now states the uniform estimate that would be
  needed. The conditional proof also makes norm equivalence and the
  differentiation of the exact area factor explicit.
- The topology explanation attributes the prescribed triangulated disk to
  equality with canonical fan incidence, rather than to preliminary
  connectivity, degree, and Euler checks alone.
- The material-flux map is described as a scaled orthogonal map. Its norm
  and the existing error bound were already correct.

The local slice argument, smooth pullback, criticality, four similarity
kernels, real Fourier normalization and conjugacy, second-variation residual
identity, corrected centers, and continuous/algebraic error transfer were
reviewed. The fixed-mesh certificates do not use the broken-regularity
conjecture or extrapolated convergence rates.

The manuscript before these edits is retained in
[Unused](Unused/torsional_rigidity_regular_polygon_before_rate_audit_20260909.tex).

## Validation code changes

Both Python certificate drivers now build private verifiers from saved C
sources in each new archive, using `certificate_build.py`. They record
executable hashes and compiler/FLINT identity. Previously, `make all` could
reuse shared executables without establishing their relationship to the
saved sources. The numerical verification formulas are unchanged.

Both drivers now require the export header to match the requested polygon,
mesh, mode, and directions. They also match the complete entry-certificate
marker, including conjugate mode and radius. Previously they counted the
marker without checking its values. The published archives already satisfy
these stronger mode/entry checks.

New C rejection tests cover incorrect canonical incidence despite valid
preliminary topology, mismatched entry directions, and independently failed
direct and conjugate containment. Valid baseline tests ensure the rejection
checks are not vacuous. The only C source edit is a topology comment;
interval arithmetic and mathematical formulas remain unchanged. NumPy is
now included in the documented dependencies.

## Evidence

The [review evidence](MaxTorsionValidation/results/validity_review_20260909/) contains:

- Successful `make -C MaxTorsionValidation/flint test`, including exact algebraic identities
  and strengthened input-rejection tests.
- Source/log audits of all 672 real-entry certificates underlying the
  published tables: 88 at m = 32, 290 at m = 64, and 294 at m = 128.
- Independent replay of all 25 published final mode tests with a fresh C
  build: eight m = 32 cases, ten m = 64 cases, and seven m = 128 cases.
  The inconclusive m = 64 cases n = 19,20 reproduce their reported counts.
- Direct checks of every published sign-margin row against the archived
  intervals, including safe outward rounding.
- Full entry replays at n = 25 for k = 1 radial/radial and k = 12 mixed
  imaginary, and at n = 24 for the tangential Nyquist entry, using saved
  fields rather than new FreeFEM candidates. All passed at the original
  references and radii.
- A checksum audit of 2701 archived files used by the tables. Exactly one
  differs from its historical checksum; its treatment is detailed below.

Both revised Python drivers also passed complete n = 5, m = 16 certificates
(eight entries, six negative eigenvalues, four exact zeros). Their archives
are [optimized](MaxTorsionValidation/results/review_20260909_guarded_rotations_n5/) and
[original candidate generation](MaxTorsionValidation/results/review_20260909_guarded_original_n5/).
Private executable and source hashes were checked. An intentionally
wrong-mode FreeFEM export was
[rejected](MaxTorsionValidation/results/review_20260909_wrong_mode_rejection/rejection_test.log)
before a result record was written. Invalid archived C source failed its
private build even though shared verifier executables existed.

The final manuscript compiles without LaTeX warnings or unresolved
references. The earlier review and its n = 3,...,10 full regeneration are
preserved in [the previous review](Unused/REVIEW_before_validity_review_20260909.md).

## Historical checksum mismatch

The compressed candidate
`extended_m128_fast_n19_onward/n21/inputs/k6_rr.txt.gz` differs from the
hash in its original archive. The cause is not established. Decompression
and its gzip integrity check succeeded. A fresh C build then independently
proved the full entry and conjugate containment at the original reference
centers and radius; the n = 21 final sign count also reproduced.

The [new replay record](MaxTorsionValidation/results/validity_review_20260909/n21-replay-provenance.json)
records compressed and uncompressed candidate hashes, verifier hash,
reference hash, and radius. Its [full entry log](MaxTorsionValidation/results/validity_review_20260909/entry-replay-n21-k6-rr.log)
is retained. The original candidate and historical checksum were preserved.
Thus the checksum discrepancy remains visible while the present candidate
has independent certification evidence. The other 2700 recorded hashes match.

## Scope and limits

This review is not formal verification of the analysis, C implementation,
compiler, operating system, or FLINT/Arb. It strengthens the evidence for
the stated computer-assisted result under that trusted computing base.
All final spectra were replayed, but the full 672-entry PDE/Arb chain was
not regenerated: four complete entries were selected for fresh replay,
including the checksum exception, and the remaining entries were audited
against saved logs, sources, and field checksums.

`summarize_extended_certificates.py` performs a source/log audit, not an
independent certificate replay. The checksums and replay in this review
are separate checks. No all-n theorem, global maximality result, proof of
broken regularity, or certification cutoff beyond n = 25 is claimed.
