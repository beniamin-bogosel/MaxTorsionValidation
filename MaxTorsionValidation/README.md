# MaxTorsionValidation: torsional-rigidity computations

Start with [Reproducing Section 8](docs/REPRODUCING_SECTION_8.md) for the
complete certificate tables through `n=25`, dependencies, fresh archive
commands, and replay instructions. This directory was previously named
`code/`; run the commands below from the repository root.

The [archive guide](results/README.md) explains which results remain local
or require a separate data bundle. Links to full historical results below
are available when that bundle is installed; a source-only Git clone
contains the compact summaries. No numerical codes were rerun for the
September 2026 repository reorganization.

`freefem/torsion_hessian.edp` is the floating-point benchmark.  It assembles
Laurain's distributed polygonal Hessian, forms the Hessian of `J/area^2`,
changes to radial/tangential coordinates, and prints the two eigenvalues of
each 2-by-2 Fourier symbol.  With `-majorant 1` it constructs both RT0
benchmark fluxes and globally H(div)-conforming, exactly equilibrated curl
fluxes for state/material residual majorants.
The old prototypes remain in `../Code/` and are not modified.

Run a single benchmark with

```sh
FreeFem++ -nw MaxTorsionValidation/freefem/torsion_hessian.edp -n 5 -m 16
```

or a refinement sequence with

```sh
FREEFEM_BIN=/path/to/FreeFem++ bash MaxTorsionValidation/freefem/run_convergence.sh 5
```

The separate [rotation implementation](freefem/ROTATIONS.md),
`freefem/torsion_hessian_rotations.edp`, uses one torsion solve and two
first-vertex solves, reconstructs all other first variations by rotation,
and assembles the Hessian from its first block row. Its full fields,
Hessians and symbols have been compared against the original benchmark.
The additional `freefem/certify_with_rotations.py` driver supplies rotated
fields and flux potentials to the existing rigorous C verifiers; see the
same instructions for the six-core continuation command.

`flint/mode_cert.c` repeats the 2-by-2 symbol calculation with Arb balls.  Its
`--radius` argument must be a proved error radius (roundoff + algebraic PDE
solve + continuous FEM error), not an empirical tolerance.  Alternatively,
`--radius-file FILE` accepts exactly one `MODE_RADIUS k radius` record for
each Fourier index.  This sharper modewise form is needed for the larger
polygons.  The option `--exact-regular-similarities` inserts the two exact
`k=0` similarity zeros, the translation zero in each of `k=1,n-1`, and the
exact zero off-diagonal at an even Nyquist mode.  It is a trusted analytic
reduction and must only be used for the scale-invariant Hessian of a regular
polygon through the validated driver below.  The separate
`flint/residual_cert.c` validates the algebraic error of an SPD finite-element
solve from its residual and a supplied lower bound for `lambda_min(K)`.

`flint/majorant_cert.c` is a conservative diagnostic for the older entrywise
continuous-error propagation.  FreeFEM exports the global P1 fields and
fitted mesh with `-export-majorant FILE`; the program reconstructs exactly
equilibrated curl fluxes and encloses all element norms with Arb.  It does not
independently prove canonical regular-fan incidence or derive the exact
boundary-node set, so it is not the theorem-grade sign path.  For example:

```sh
FreeFem++ -nw MaxTorsionValidation/freefem/torsion_hessian.edp -n 5 -m 32 \
  -majorant 1 -export-majorant /tmp/torsion-majorant.txt
MaxTorsionValidation/flint/majorant_cert /tmp/torsion-majorant.txt --prec 192 \
  --geom-radius 1e-13
```

`flint/second_variation_cert.c` evaluates the sharp residual product bound
from the manuscript's second-variation residual identity
(`eq:second-residual-bound`).  Each input line has
`q r delta0 deltaq deltar etaqr Cqr` (the last field was formerly called
`Mqr`); unlike the older entrywise propagation,
all its FEM-error contributions are quadratic in first-order residual
radii.

`freefem/second_variation_lifting.edp` assembles that lifting for a chosen
real Fourier-mode pair and exports its fields.  `flint/second_lifting_cert.c`
then proves the canonical integer fan incidence, proves that the exported
geometry/directions enclose the exact regular data, regenerates those data
analytically with Arb, and replays the calculation on the exact interior
Dirichlet submatrices.  Floating solves are only candidates: four interval
residuals correct both the solution errors and the Hessian center.  The
output is one continuous-plus-algebraic enclosure for the scale-invariant
Hessian entry.  For example:

```sh
FreeFem++ -v 0 -nw MaxTorsionValidation/freefem/second_variation_lifting.edp \
  -n 5 -m 32 -k 2 -qa 0 -qp 0 -ra 0 -rp 0 \
  -export-second /tmp/second-k2-rr.txt
MaxTorsionValidation/flint/second_lifting_cert /tmp/second-k2-rr.txt --prec 192 \
  --geom-radius 1e-13 --direction-radius 1e-13 --check-regular-inputs
```

`--check-regular-inputs` is mandatory for certificate output.  The explicit
`--diagnostic-nonstrict` alternative is only for non-certifying experiments.

The optimized curl-potential solves do not themselves require validation:
any rounded continuous P1 potential remains an admissible exactly
equilibrated flux.  Optimization changes only the size of the majorant.

Build and smoke-test all five programs with `make -C MaxTorsionValidation/flint test`.

The complete validated run for every regular polygon from the triangle to
the decagon is

```sh
FREEFEM_BIN=/path/to/FreeFem++ \
  bash MaxTorsionValidation/freefem/run_regular_certificates.sh 3 10
```

It uses the uniform fitted fan resolution `m=32`.  Each independently
certified entry and its conjugate must lie in the exact same center-radius
ball later consumed by `mode_cert`; any missing entry, failed containment,
or wrong eigenvalue count terminates the run.  The final assertions are
exactly `2*n-4` negative eigenvalues and four algebraically known similarity
zeros.  The equivalent long integration-test target is
`make -C MaxTorsionValidation/flint test-regular`.

To retain every per-entry enclosure, every reference-center and radius file,
the final Arb mode logs, software versions, and source hashes, give a new
archive path:

```sh
CERT_ARCHIVE_DIR=MaxTorsionValidation/results/n3_n10_certificate_archive \
  bash MaxTorsionValidation/freefem/run_regular_certificates.sh 3 10
```

New archives also retain the exact exported candidate fields in compressed
`inputs/*.txt.gz` files and relative `SHA256SUMS`. This permits replay of the
Arb verification without repeating the FreeFEM solves. Both containment and
final mode evaluation use 192 bits, ensuring identical reference balls.

The driver refuses an existing archive path so a previous record cannot be
silently overwritten.  It writes `COMPLETE` only after every entry
containment and every asserted eigenvalue count succeeds.

To test larger polygons or another mesh resolution without changing the
stored radii of the main driver, use the exploratory driver:

```sh
python3 MaxTorsionValidation/freefem/test_extended_certificates.py 11 21 --m 64 \
  --jobs 4 --cpu-cores 4 --entry-centers --stop-on-failure \
  --archive MaxTorsionValidation/results/new_extended_run
```

It computes new radii, then has `second_lifting_cert` rigorously check every
entry and conjugate against the precise balls consumed by `mode_cert`, at
192 bits. With `--entry-centers`, the reference centers come from the
lifting calculations; otherwise they come from the full FreeFEM Hessian.
The decimal radius calculation only proposes a bound: successful Arb
containment checks are required before any sign certificate is reported.
The PDE-solve tolerances and rigorous error formulas are unchanged. The
verifier nominates lattice indices by inverting the sector coordinate maps;
all Arb geometry, bijection, boundary, and incidence checks remain mandatory.
If the auxiliary
curl-potential fit stalls at the default relative tolerance `1e-13`,
`--flux-eps 1e-9` relaxes only that optimization tolerance. This changes the
candidate flux and potentially the sharpness of the measured majorant;
every resulting enclosure must still pass the same rigorous verification.

All subprocesses inherit the CPU affinity limit and low scheduling priority;
numerical-library threading is limited to one thread. The default is one
worker on one logical CPU. `--cpu-cores` limits logical CPUs; on this
six-core machine, CPUs 0--5 correspond to six distinct physical cores.
The entry queue spans all representative modes, so `--jobs 6 --cpu-cores 6`
uses at most six workers. The complete reference file is written once and
remains unchanged during all entry-containment checks.
Each completed polygon has a `RESULT.json` and `mode_cert.log`, and the
archive retains compressed inputs and source copies. `FINISHED` means the
sweep finished, including a possible failed sign test; consult
`results.jsonl` for the actual outcomes. Failure to certify is not a
counterexample to local maximality. `--stop-on-failure` stops at the first
failed full sign test and does not establish monotonicity in the number of
sides.

The completed `m=64` tests for `n=11,...,20`, including the successful
certificates through `n=18` and unresolved sign tests at `n=19,20`, are
recorded in [the extension-test summary](results/extended_m64_summary.md).
The completed `m=128` certificates for `n=19,...,25` are recorded in
[the finer-mesh summary](results/extended_m128_summary.md). The optimized
continuation includes the separately requested `n=25` certificate and stops there.
`freefem/summarize_extended_certificates.py` audits completed archive records
and produces JSON table data. It checks source hashes, entry-containment
markers, matching radii, and final inertia counts, and rounds displayed
upper endpoints toward positive infinity.
This is an audit of saved logs and sources; it does not rerun the Arb
entry checks. Independent replay additionally uses the archived compressed
fields, reference centers, and radii.

The two Python certificate drivers build private verifiers from their
archived C sources in a fresh `build/` directory. Their manifests record
the executable hashes and compiler/FLINT identity, and each exported entry
must match the requested polygon, mesh, mode, and directions. The shared
executables in `MaxTorsionValidation/flint/` are not used by these runs.

The full pentagon run (eight real mode-entry liftings followed by the Arb
eigenvalue test) is automated by

```sh
FREEFEM_BIN=/path/to/FreeFem++ \
  bash MaxTorsionValidation/freefem/run_pentagon_certificate.sh
```

For every entry, the driver proves that its computed enclosure and conjugate
are contained in the stored benchmark center plus the common radius
`4.9e-4`. It passes that same radius to `mode_cert`, which exits successfully
only for exactly five symbols, six certified negative eigenvalues, and the
four similarity zeros inserted analytically (reported by the raw status
counter as zero-containing, hence unresolved, balls).

The checked reference output is summarized in
`results/pentagon_certificate_summary.txt`.
The all-polygon run, per-mode radii, and final counts are summarized in
`results/n3_n10_certificate_summary.txt`.

`symbolic/equilateral_sector_integrals.py` uses exact SymPy integration to
verify the closed formulas for $X,Y,Z$ in the circumradius-one equilateral
case:

```sh
python3 MaxTorsionValidation/symbolic/equilateral_sector_integrals.py
```

The other exact symbolic tests verify the planar Laurain-to-`(HJ)` reduction
and every first/mixed pullback coefficient used by the C verifier.  Run all
four with `make -C MaxTorsionValidation/flint test-symbolic`.
