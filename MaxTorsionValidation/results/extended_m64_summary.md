# Regular-polygon extension tests at m = 64

Tests completed for every n from 11 through 20. With the present continuous-error enclosures, one common entry radius per Fourier mode, and the existing `mode_cert` eigenvalue calculation, the largest certified polygon in this range is **n = 18**. Every n from 11 through 18 passes; n = 19 and n = 20 do not pass the full sign test.

This is an observed limit of this implementation on this mesh, not a proof that larger polygons cannot be certified by sharper bounds or another validated sign test. No manuscript changes were made for this experiment.

| n | Triangles | Certified negative / required | Additional unresolved eigenvalues | Largest nonsimilarity upper endpoint | Result |
|---:|---:|---:|---:|---:|---|
| 11 | 45056 | 18 / 18 | 0 | -2.116539E-4 | [PASS](extended_m64_n11/n11/mode_cert.log) |
| 12 | 49152 | 20 / 20 | 0 | -1.504192E-4 | [PASS](extended_m64_n12_n16/n12/mode_cert.log) |
| 13 | 53248 | 22 / 22 | 0 | -1.086377E-4 | [PASS](extended_m64_fourcores_n13_n21/n13/mode_cert.log) |
| 14 | 57344 | 24 / 24 | 0 | -7.997360E-5 | [PASS](extended_m64_fourcores_n13_n21/n14/mode_cert.log) |
| 15 | 61440 | 26 / 26 | 0 | -5.972883E-5 | [PASS](extended_m64_fourcores_n13_n21/n15/mode_cert.log) |
| 16 | 65536 | 28 / 28 | 0 | -4.519143E-5 | [PASS](extended_m64_fourcores_n13_n21/n16/mode_cert.log) |
| 17 | 69632 | 30 / 30 | 0 | -3.445156E-5 | [PASS](extended_m64_fourcores_n13_n21/n17/mode_cert.log) |
| 18 | 73728 | 32 / 32 | 0 | -2.641392E-5 | [PASS](extended_m64_fourcores_n13_n21/n18/mode_cert.log) |
| 19 | 77824 | 30 / 34 | 4 | 1.348336E-5 | [NOT CERTIFIED](extended_m64_n19_n20_flux1e9/n19/mode_cert.log) |
| 20 | 81920 | 28 / 36 | 8 | 1.688706E-4 | [NOT CERTIFIED](extended_m64_n19_n20_flux1e9/n20/mode_cert.log) |

The four analytically known similarity zeros are excluded from the unresolved column. Upper endpoints are conservative values obtained from the printed Arb balls, rounded upward; the unrounded Arb sign decisions are authoritative. All sign calculations use 192-bit arithmetic.

At n = 19, the unresolved upper branches are k = 6, 7, 12, 13. At n = 20 they are k = 6, 7, 8, 9, 11, 12, 13, 14. Their intervals contain zero; no positive eigenvalue is proved. At n = 18, all 32 required eigenvalues are negative, on 73,728 triangles.

## Candidate generation and numerical checks

- n = 11,12 used reference centers from the full FreeFEM Hessian. n = 13 through 20 used reference centers from the modewise lifting calculations, avoiding the full floating Hessian assembly. Every entry and its conjugate were independently checked against the exact same reference balls subsequently read by `mode_cert`.
- The original auxiliary curl fit stalled at n = 19, k = 2, tangential/tangential. The optional `-flux-eps` parameter was added to `second_variation_lifting.edp`; its default remains 1e-13. The completed n = 19,20 tests use 1e-9 for this auxiliary fit only. The state/material/lifting PDE tolerances remain 1e-13, and both C verifiers are unchanged. Any rounded continuous P1 potential defines an admissible flux; the measured residual majorant is still rigorously recomputed.
- After the CPU-limit request, tests ran at niceness 10 on logical CPUs 0--3, which are four distinct physical cores of this six-core machine. Physical cores 4 and 5 and their sibling threads were excluded. Numerical-library threading was limited to one thread. All test processes have finished.
- Archived source hashes, the expected entry counts, exact-mesh and exact-input checks, equality of containment radii with the final radius files, and the final mode counts were audited for every completed polygon.

## Reproduction

```sh
python3 MaxTorsionValidation/freefem/test_extended_certificates.py 11 20 --m 64 \
  --jobs 4 --cpu-cores 4 --entry-centers --flux-eps 1e-9 \
  --archive MaxTorsionValidation/results/new_m64_extension
```

This command runs the current pipeline uniformly with lifting centers and the relaxed auxiliary tolerance. To reproduce the original batches exactly, consult each archive's `MANIFEST.json` and `sources/`; the candidate inputs are retained as `inputs/*.txt.gz`. Changing how reference centers are proposed does not relax any containment check.

## Completed records

- n = 11: [n11](extended_m64_n11/n11/), 20 entry certificates, 330.1 seconds for this full run.
- n = 12: [n12](extended_m64_n12_n16/n12/), 22 entry certificates, 477.8 seconds for this full run.
- n = 13: [n13](extended_m64_fourcores_n13_n21/n13/), 24 entry certificates, 135.2 seconds for this full run.
- n = 14: [n14](extended_m64_fourcores_n13_n21/n14/), 26 entry certificates, 175.5 seconds for this full run.
- n = 15: [n15](extended_m64_fourcores_n13_n21/n15/), 28 entry certificates, 190.1 seconds for this full run.
- n = 16: [n16](extended_m64_fourcores_n13_n21/n16/), 30 entry certificates, 229.9 seconds for this full run.
- n = 17: [n17](extended_m64_fourcores_n13_n21/n17/), 32 entry certificates, 220.0 seconds for this full run.
- n = 18: [n18](extended_m64_fourcores_n13_n21/n18/), 34 entry certificates, 254.3 seconds for this full run.
- n = 19: [n19](extended_m64_n19_n20_flux1e9/n19/), 36 entry certificates, 264.9 seconds for this full run.
- n = 20: [n20](extended_m64_n19_n20_flux1e9/n20/), 38 entry certificates, 317.3 seconds for this full run.

Other `extended_m64*` directories contain interrupted exploratory work. They have explicit interruption/stop notes and must not be mistaken for completed certificates. In particular, the partial n = 21 run has no full sign result.
