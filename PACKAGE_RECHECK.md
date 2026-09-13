# Final package recheck — 13 September 2026

The recheck found no new discrepancy affecting the reported certificates
through n = 25. The reference requested by the author was absent and has
been added to the introduction and bibliography:

Joel Dahne, Javier Gómez-Serrano, and Joana Pech-Alberich,
*Monotonicity of the first Dirichlet eigenvalue of regular polygons*,
[arXiv:2601.16285](https://arxiv.org/abs/2601.16285), 2026.

The introduction describes its equal-area spectral monotonicity theorem
and the combination of rigorous asymptotic bounds with validated numerical
enclosures. It does not attribute a fixed-n torsion maximality theorem to
that reference.

The current manuscript and bibliography were backed up in
`Unused/final_package_review_20260913/` before editing. Small corrections
also fix the `DetailedExplanations` folder name, PDF author metadata, an
omitted verb, and one overflowing sentence. The author's new AI-assistance
discussion and the mathematical statements are retained.

## Checks performed

| Check | Result |
| --- | --- |
| Manuscript and four course PDFs | Clean builds, including a fresh course build without the manuscript or bulk data |
| Fresh build and complete `make -B -C MaxTorsionValidation/flint test` | Passed, including exact algebra, input rejection, residual, flux, and mode fixtures |
| Syntax of maintained Python and shell programs | Passed: 13 Python files and 4 shell scripts |
| Both certificate drivers in a source-only copy, n = 5, m = 16 | Both `CERTIFIED`: six negative eigenvalues and four exact similarity zeros |
| Rotation implementation compared with the original full-solve benchmark | All seven test cases passed the specified 1e-10 tolerance |
| Published entry-record and source-snapshot audit | 672 entry records passed |
| Fresh C replay of final spectral checks | All 25 published mode counts reproduced, including the two inconclusive m = 64 cases |
| Manuscript sign-margin tables | All 25 rows agree with the archived enclosures and outward rounding |
| Full n = 25, m = 128, k = 1 radial/radial entry replay | Entry and conjugate containment passed at the original radius |
| Historical archive hashes | 2701 checked; exactly the previously documented n = 21 exception, with no new mismatch |

The source-only copy contains the Git-selected sources and course inputs,
without `theory/`, third-party papers, or the bulk result archives. Both
drivers built private verifiers and completed fresh certification from
that copy. This checks the reorganized paths as well as the executable
build chain. Existing computation archives were read without modification.

The manuscript and all four course PDFs are rebuilt. The course's copied
bibliography, manuscript equation-number map, generated result tables, and
source hashes are refreshed to match the current project. The publishing
instructions now include the course and no longer describe Git metadata as
an empty placeholder.

## Evidence and limits

Detailed output is retained locally in
`MaxTorsionValidation/results/package_recheck_20260913/`:

- `make-test.log`, `syntax-check.log`, `manuscript-build.log`, `course-build.log`, `source-only-course-build.log`, and `document-checks.log`;
- `source-only-snapshot.json`, `optimized_n5_m16/`, and `original_n5_m16/`;
- `rotation_comparison/` and `rotation-comparison.log`;
- `archive-audit.log`, `archive-audit-result.json`, `m64-audit.json`, and `m128-audit.json`;
- `mode-replay-m*-n*.log` and `mode-replay-build.json`;
- `entry_n25_k1_rr/`, including the entry log, copied verifier source, and input/executable/reference hashes;
- `archive-integrity.json` and `archive-hash-mismatches.json`.

As before, the compressed n = 21, k = 6 radial/radial field differs from its
original checksum. The file is unchanged from the independently recertified
version: its compressed and uncompressed hashes, reference hash, and radius
match `validity_review_20260909/n21-replay-provenance.json`. The original
checksum remains visible. This is a documented exception, not an assertion
that every historical checksum matches.

This recheck is not a regeneration of all 672 finite-mesh entry proofs or
a formal verification of the analysis. It combines the recorded-entry/hash
audit, all final mode replays, one full fine-mesh entry replay, and two fresh
small certification chains. It does not alter the certified range, prove
the uniform-neighborhood regularity conjecture, or give a global maximality
result.

The main manuscript remains outside the Git source distribution under the
existing `.gitignore`. The manuscript/PDF and the bulk certificate evidence
must accompany a complete research release separately; the course LaTeX
and PDFs are part of the Git package. No commit or push is performed by this
recheck.
