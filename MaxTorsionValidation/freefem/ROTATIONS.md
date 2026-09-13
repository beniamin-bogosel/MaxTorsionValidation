# Three-solve Hessian benchmark

`torsion_hessian_rotations.edp` is a separate implementation. The original
`torsion_hessian.edp` and the validated lifting codes are retained.

For each mesh, it solves torsion once and the two first-variation PDEs at
vertex `(1,0)` once. It then uses the manuscript's covariance identity
(`eq:dihedral-covariance`):

$$
U_j(x)=R_jU_0(R_j^T x),
\qquad
R_j=\begin{pmatrix}
\cos(j\vartheta)&-\sin(j\vartheta)\\
\sin(j\vartheta)&\cos(j\vartheta)
\end{pmatrix},
\qquad \vartheta=\frac{2\pi}{n}.
$$

Both the spatial argument and the two direction components are rotated.
Rotating the argument alone would give radial/tangential directions at
vertex `j`, rather than its Cartesian directions.

The mesh is the same uniform refinement of the coarse fan. Each node is
identified by its sector and integer coordinates within that sector.
Rotation changes the sector index. The code checks the resulting node
permutation, its coordinate error, and return to the original node after
`n` rotations. It copies nodal coefficients using this permutation; it does
not interpolate the solution onto another mesh. The measured torsion
symmetry defect is also printed.

Only the first Hessian block row is assembled. Its Gram terms use two
stiffness-matrix products; the local geometric terms involve only vertices
`0`, `1`, and `n-1`. Rotation then supplies the remaining block rows.
The normalization by area and the `SUMMARY`, `MODE`, and `MODE_DATA` output
formats agree with the original benchmark.

Run from the repository root:

```sh
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 taskset -c 0-5 \
  FreeFem++ -v 0 -nw MaxTorsionValidation/freefem/torsion_hessian_rotations.edp -n 21 -m 128
```

On this machine, CPUs `0–5` are six distinct physical cores. The code prints
one `PDE_SOLVED` record for each solve and `PDE_SOLVES ... total=3`.

The [comparison script](test_torsion_hessian_rotations.py) checks all
first-variation fields, the full Hessian, and every Fourier symbol against
the original code. It only adds output instrumentation to temporary source
copies:

```sh
python3 MaxTorsionValidation/freefem/test_torsion_hessian_rotations.py \
  --archive /tmp/torsion-rotation-review
```

The [completed review](../results/rotation_benchmark_final_review/README.md)
covers seven cases, including odd and even polygons, a non-dyadic mesh,
and `n=21`. The largest first-variation difference was below `1.8e-14`;
the largest full-Hessian difference was below `1.1e-15`. At `n=21,m=16`,
the single-core comparison took 52.2 seconds for the original code and
0.93 seconds for this implementation. The additional `n=21,m=128` floating
run completed in 70.34 seconds with three PDE solves.

This benchmark computes floating Hessian data. The separate
`certify_with_rotations.py` now uses the same first-vertex rotation approach
to generate candidates for the existing rigorous lifting verifier.

With `--entry-centers`, its `second_variation_lifting_rotations.edp` solves
torsion and the two first-vertex PDEs once, together with three auxiliary
flux fits. `rotation_cache.py` forms the normalized real Fourier
combinations of the rotated fields and flux potentials. Each entry then
requires one second lifting and its auxiliary flux fit. The first-variation
reduction does not remove these second liftings from the error estimate.

Every candidate is still checked by the unchanged C programs: exact mesh
and directions, continuous and algebraic residuals, entry and conjugate
containment, and the complete eigenvalue count. NumPy only proposes the
cached fields; the certificate does not rely on its arithmetic being
rigorous. The exported fields retain everything required for Arb replay.
The [integration checks](../results/rotation_certificate_integration_review/README.md)
include full odd- and even-polygon certificates.

The completed continuation certified `n=22,23,24` and then the separately
requested `n=25`, using six physical cores; see the
[validated results](../results/extended_m128_summary.md). The command to
repeat this range, stopping earlier if a certificate fails, is:

```sh
python3 MaxTorsionValidation/freefem/certify_with_rotations.py 22 25 --m 128 \
  --jobs 6 --cpu-cores 6 --entry-centers --flux-eps 1e-9 \
  --stop-on-failure --archive /tmp/torsion-rotations-m128
```
