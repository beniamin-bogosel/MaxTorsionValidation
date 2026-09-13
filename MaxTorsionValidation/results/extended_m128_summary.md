# Completed certificates at m = 128

Every polygon from n = 19 through n = 25 is certified. The optimized continuation tested n = 22 through n = 24, then n = 25 in a separate run, using at most six physical cores. All full certificates passed. No polygon beyond n = 25 was attempted.

| n | Triangles | Negative / required | Upper bound for every nonsimilarity eigenvalue | Result |
|---:|---:|---:|---:|:---|
| 19 | 311296 | 34 / 34 | -0.0000273 | [CERTIFIED](extended_m128_fast_n19_onward/n19/mode_cert.log) |
| 20 | 327680 | 36 / 36 | -0.0000222 | [CERTIFIED](extended_m128_fast_n19_onward/n20/mode_cert.log) |
| 21 | 344064 | 38 / 38 | -0.0000181 | [CERTIFIED](extended_m128_fast_n19_onward/n21/mode_cert.log) |
| 22 | 360448 | 40 / 40 | -0.0000149 | [CERTIFIED](extended_m128_rotations_n22_n30/n22/mode_cert.log) |
| 23 | 376832 | 42 / 42 | -0.0000124 | [CERTIFIED](extended_m128_rotations_n22_n30/n23/mode_cert.log) |
| 24 | 393216 | 44 / 44 | -0.0000103 | [CERTIFIED](extended_m128_rotations_n22_n30/n24/mode_cert.log) |
| 25 | 409600 | 46 / 46 | -0.00000871 | [CERTIFIED](extended_m128_rotations_n25/n25/mode_cert.log) |

The four exact similarity zeros are excluded. Printed upper bounds are rounded toward positive infinity. At m = 64, n = 19 and n = 20 were inconclusive; m = 128 resolves both tests.

The [subsequent validity review](../../REVIEW.md) reproduced all final sign counts. One historical checksum differs for `extended_m128_fast_n19_onward/n21/inputs/k6_rr.txt.gz`; that complete entry was independently recertified with a fresh C build at its original reference and radius. The [new replay record](validity_review_20260909/n21-replay-provenance.json) records the current candidate and verifier hashes. The original archive and checksum were preserved.

## Optimized candidate generation and rigorous checks

The continuation uses `certify_with_rotations.py`, `second_variation_lifting_rotations.edp`, and `rotation_cache.py`. With `--entry-centers`, each polygon uses one torsion solve, two first-vertex derivative solves, and three auxiliary flux fits. A checked nodal permutation and real Fourier combinations supply all first-variation fields and their flux potentials. Each real entry still has its second lifting and one auxiliary flux fit.

The C verifiers and the rigorous error formulas are unchanged. They check the exact regular fan and Fourier directions, compute continuous and algebraic residual bounds, prove each entry and conjugate containment, and then check the complete eigenvalue count. NumPy generates candidates only. Every final export contains the fields needed for independent Arb replay.

All runs use 192-bit Arb, geometry and direction radii `1e-13`, PDE-solve tolerance `1e-13`, and auxiliary curl-fit tolerance `1e-9`. No mesh-convergence estimate is substituted for a validated bound.

## Archives and reproduction

The n = 19,20,21 certificates are in `extended_m128_fast_n19_onward`; its interrupted n = 22 folder is not a completed test. The optimized n = 22,23,24 continuation is in `extended_m128_rotations_n22_n30`; the additional n = 25 certificate is in `extended_m128_rotations_n25`, with identical numerical sources and validation settings. Source snapshots, manifests, compressed fields, exact-input and containment logs, reference centers, radii, final eigenvalue logs and checksums are archived. The earlier `extended_m128_sixcores_n22_onward` run was stopped before any full result and is excluded.

The rotation integration was checked on complete n = 5 and n = 6 certificates; see `rotation_certificate_integration_review`. Common entry radii changed by less than 1.5e-12 relatively compared with the original candidate path. The floating benchmark comparisons are separate, in `rotation_benchmark_final_review`.

```sh
python3 MaxTorsionValidation/freefem/certify_with_rotations.py 22 25 --m 128 \
  --jobs 6 --cpu-cores 6 --entry-centers --flux-eps 1e-9 \
  --stop-on-failure --archive /tmp/torsion-rotations-m128
```

On this host, CPUs 0–5 are six distinct physical cores; hyperthread siblings 6–11 are excluded. Library threading is one. The original n = 19,20,21 runs initially used four cores; their allocation history is recorded in `EXECUTION_NOTES.md`. These timings are not controlled cross-method benchmarks.

The processor is an Intel Core i7-9750H (2.60 GHz nominal frequency). The optimized n = 22,...,25 certificates took about 13–20 minutes per polygon, including candidate generation, all entry and eigenvalue checks, and field compression. Their recorded times total 66.4 minutes; allow roughly 70 minutes for the four-polygon command above on comparable hardware. The n = 25 run alone took 1059.4 seconds (17.7 minutes). These are wall-clock estimates and depend on other machine activity; the driver records its polygon time before the final archive checksum pass.

The upper limit was revised from 30 to 24 while n = 24 was running. Its remaining containment checks were completed by the bounded `finish_n24.py` in the archive. The n = 24 time includes this controlled handoff; see [the continuation record](extended_m128_rotations_n22_n30/CONTINUATION.md). No n = 25 computation was started by that sweep. The subsequent, separately requested n = 25 run used the same command with the range `25 25` and a new archive path.

| n | Full run seconds |
|---:|---:|
| 19 | 942.0 |
| 20 | 1079.5 |
| 21 | 1248.6 |
| 22 | 809.2 |
| 23 | 950.4 |
| 24 | 1164.1 |
| 25 | 1059.4 |
