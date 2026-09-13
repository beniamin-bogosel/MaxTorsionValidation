# Reproducing the certificates in Section 8

This guide covers the manuscript section **Validated certificates for regular
polygons**, whose stable LaTeX label is `sec:n3-n10`. The label predates the
extension through 25 sides. It covers the tables at mesh resolutions `m=32`,
`m=64`, and `m=128` and the computer-assisted result for `5 <= n <= 25`.
The triangle and square are consistency checks.

All commands below are instructions for a future run, issued from the
repository root. They were not executed during the publication-packaging
update. The manuscript in the local `theory/` folder is not part of this
Git publication.

## Environment and setup

The certificate drivers support Linux, including an appropriately configured
Linux installation under WSL. The Python sweep drivers use
`os.sched_getaffinity`, `os.sched_setaffinity`, and `os.nice`; they are not
portable unchanged to native Windows or macOS.

Install the following through your system package manager or their upstream
installation instructions:

- A C11 compiler, GNU make, and FLINT 3 with its development headers and
  library. The programs include `flint/arb.h` and link with `-lflint -lm`.
- FreeFEM, including the `lapack` plugin and `macro_ddm.idp` include file.
- Python 3 with NumPy for the rotation-based driver. SymPy is needed only
  for the symbolic checks.
- Bash, gzip, and the usual GNU command-line utilities, including `find`,
  `sort`, `xargs`, and `sha256sum`. `pkg-config` supplies optional version
  metadata in the shell driver.

The saved runs used FreeFEM 4.15, FLINT 3.0.1, and GCC 13.3. These are
recorded reference versions, not a claim that every other version produces
identical floating-point candidates.

An optional Python environment and a build of the maintained C programs are:

```sh
python3 -m venv .venv
. .venv/bin/activate
python3 -m pip install -r MaxTorsionValidation/requirements.txt
make -B -C MaxTorsionValidation/flint all
```

If FreeFEM is not on `PATH`, set its executable explicitly before running
any driver:

```sh
export FREEFEM_BIN=/absolute/path/to/FreeFem++
```

The shell driver builds the maintained C executables. Each Python driver
instead snapshots its sources and builds private verifiers in its new
archive, recording the executable hashes and compiler/FLINT identity.
The optional regression suite is:

```sh
make -C MaxTorsionValidation/flint test
```

This suite checks symbolic identities, small validated examples, and rejected
invalid inputs; it does not regenerate the Section 8 certificate tables.

## Start with one small certificate

No historical candidate archive is needed to regenerate a proof from the
source code. After installing the dependencies above, the optimized driver
can generate and certify the pentagon on the `m=32` mesh:

```sh
python3 MaxTorsionValidation/freefem/certify_with_rotations.py \
  5 5 --m 32 --jobs 2 --cpu-cores 2 --entry-centers \
  --flux-eps 1e-13 --stop-on-failure \
  --archive /tmp/torsion-pentagon
```

The archive directory must be new. Success requires eight real entry
certificates, six negative Hessian eigenvalues, and four exact similarity
zeros. Check `n5/RESULT.json` for `CERTIFIED` and consult the entry logs and
`n5/mode_cert.log`. This command computes the PDE candidates, proves their
error bounds, and generates its own compressed inputs and audit records.
It does not require the multi-gigabyte historical data collection.

Regeneration can produce different floating-point candidates and radii;
the new certificate establishes its own enclosures. Replaying the exact
historical inputs is a separate task requiring those input files. The
following batches regenerate all reported table cases.

## Regenerate candidates and certify all table cases

The following four batches cover every row. Create the parent directory once:

```sh
mkdir -p MaxTorsionValidation/results/section8-new
```

Every child archive path below **must be new**. The drivers refuse an
existing path. Choose another parent name for a later repetition; do not
overwrite a completed archive.

For the triangle through decagon, use the calibrated `m=32` shell driver:

```sh
CERT_ARCHIVE_DIR=MaxTorsionValidation/results/section8-new/m32-n3-n10 \
  bash MaxTorsionValidation/freefem/run_regular_certificates.sh 3 10
```

For all ten rows of the `m=64` extension, including its two inconclusive
cases:

```sh
python3 MaxTorsionValidation/freefem/test_extended_certificates.py \
  11 20 --m 64 --jobs 4 --cpu-cores 4 \
  --entry-centers --flux-eps 1e-9 \
  --archive MaxTorsionValidation/results/section8-new/m64-n11-n20
```

For the first three rows of the `m=128` table:

```sh
python3 MaxTorsionValidation/freefem/test_extended_certificates.py \
  19 21 --m 128 --jobs 4 --cpu-cores 4 \
  --entry-centers --flux-eps 1e-9 \
  --archive MaxTorsionValidation/results/section8-new/m128-n19-n21
```

For the final four rows, use the rotation-based candidate generator:

```sh
python3 MaxTorsionValidation/freefem/certify_with_rotations.py \
  22 25 --m 128 --jobs 6 --cpu-cores 6 \
  --entry-centers --flux-eps 1e-9 --stop-on-failure \
  --archive MaxTorsionValidation/results/section8-new/m128-n22-n25
```

`--jobs` limits simultaneous entry calculations; `--cpu-cores` limits the
available logical CPUs for the whole run. Reduce both for a smaller machine.
The earlier interpretation of six logical CPUs as six distinct physical
cores was specific to the recorded machine. The drivers limit numerical
library threading to one thread per process.

All four batches use 192-bit Arb arithmetic and geometry/direction radii
`1e-13`. The state, first-variation, and lifting solve tolerances remain
`1e-13`. `--flux-eps 1e-9` changes only the auxiliary curl-potential fit:
its resulting flux and all residual bounds are still independently checked.
`--entry-centers` takes proposed centers from the lifting calculations.
Successful interval containment is required before the final spectrum test.

## Expected results and interpretation

| Batch | Real entry certificates | Archived final result |
| --- | ---: | --- |
| `m=32`, `n=3,...,10` | 88 | Every polygon has `2*n-4` negative eigenvalues and four exact similarity zeros. |
| `m=64`, `n=11,...,20` | 290 | Every polygon through 18 passes. At 19, 30 of 34 required negative eigenvalues are proved; at 20, 28 of 36 are proved. |
| `m=128`, `n=19,...,21` | 114 | 34, 36, and 38 negative eigenvalues, respectively; four exact zeros each. |
| `m=128`, `n=22,...,25` | 180 | 40, 42, 44, and 46 negative eigenvalues, respectively; four exact zeros each. |

The four exact zeros are supplied by the proved similarity identities.
The verifier's raw `unresolved` counter includes these four zeros. Thus a
successful final assertion requires `--expect-negative 2*n-4` and
`--expect-unresolved 4`. For the archived inconclusive `m=64` tests, the
raw unresolved counts are 8 at `n=19` and 12 at `n=20`.

The shell archive writes `COMPLETE` only after all entry containments and
all final inertia assertions succeed. A Python sweep writes `FINISHED`
when it finishes, even if a final sign test was inconclusive. Inspect
`results.jsonl`, each polygon's `RESULT.json`, and `mode_cert.log` for the
actual result. Do not add `--stop-on-failure` to the `m=64` command if both
inconclusive rows are to be reproduced. An inconclusive enclosure is not a
counterexample to local maximality.

These are the recorded results. Fresh solves may propose slightly different
centers, fluxes, radii, and timings because of software or hardware rounding.
The current uniform `m=64` command also uses lifting centers and the relaxed
auxiliary tolerance throughout, whereas the original batches introduced
these choices at different points. Each newly reported certificate must
pass its own complete containment and sign checks.

## Historical evidence and replay

The Git publication includes source code and result summaries. Bulk
certificate archives are excluded from Git and remain local or must be
obtained as a separately distributed data bundle. No public archive download
URL has been assigned. The commands above regenerate their own archives
without downloading historical results.

The original table records are located under `MaxTorsionValidation/results/`
in the following archive directories:

| Table cases | Archive directory |
| --- | --- |
| `m=32`, `n=3,...,10` | `review_2026_09_08_certificate_archive` |
| `m=64`, `n=11` | `extended_m64_n11` |
| `m=64`, `n=12` | `extended_m64_n12_n16` |
| `m=64`, `n=13,...,18` | `extended_m64_fourcores_n13_n21` |
| `m=64`, `n=19,20` | `extended_m64_n19_n20_flux1e9` |
| `m=128`, `n=19,...,21` | `extended_m128_fast_n19_onward` |
| `m=128`, `n=22,...,24` | `extended_m128_rotations_n22_n30` |
| `m=128`, `n=25` | `extended_m128_rotations_n25` |

Archive names can include a planned range that was not completed. Only
the cases identified above underlie the tables. The older
`n3_n10_certificate_archive` lacks saved candidate fields; use the September
8 archive for complete `m=32` replay.

Preserve each archive's source snapshots, manifests, compressed candidate
fields, reference centers, radii, entry-containment logs, final eigenvalue
logs, result records, and historical checksums. The folder rename does not
alter these historical files; embedded `code/` paths refer to their original
layout. The source/log audit utility
`freefem/summarize_extended_certificates.py` checks saved records but does not
recompute the PDE error bounds.

Replay takes the archived decimal fields, centers, and radii as fixed input
and reruns the rigorous C verification, avoiding new FreeFEM solves. This
fixes the numerical inputs; bit-for-bit output across different compilers or
FLINT versions is not promised. Full replay of all table evidence requires
all 672 entry verifications and the 25 final spectrum tests. A final spectrum
test alone assumes that the input entry radii have already been certified.

There is one documented historical checksum exception:
`extended_m128_fast_n19_onward/n21/inputs/k6_rr.txt.gz` differs from its
original recorded hash. The original hash was preserved. A later independent
replay certified that entry and its conjugate at the original reference
centers and radius. Its evidence is retained in
`validity_review_20260909/n21-replay-provenance.json` and
`validity_review_20260909/entry-replay-n21-k6-rr.log`. Distribute this evidence
with the affected archive; do not silently replace the historical checksum.

### Example: replay one `n=25` entry and its final spectrum

This example requires the separately available `n=25` archive. Set
`archive_dir` to its actual location if it is not under the results folder.
It compiles the two archived verifier sources into a fresh temporary
directory and leaves both the archive and its historical logs unchanged.

```sh
set -euo pipefail
archive_dir="$PWD/MaxTorsionValidation/results/extended_m128_rotations_n25"
replay_dir=$(mktemp -d "${TMPDIR:-/tmp}/torsion-n25-replay.XXXXXX")

cc -O2 -std=c11 "$archive_dir/sources/second_lifting_cert.c" \
  -o "$replay_dir/second_lifting_cert" -lflint -lm
cc -O2 -std=c11 "$archive_dir/sources/mode_cert.c" \
  -o "$replay_dir/mode_cert" -lflint -lm

gzip -dc "$archive_dir/n25/inputs/k1_rr.txt.gz" > "$replay_dir/input.txt"

"$replay_dir/second_lifting_cert" "$replay_dir/input.txt" \
  --prec 192 --geom-radius 1e-13 --direction-radius 1e-13 \
  --check-regular-inputs --reference "$archive_dir/n25/modes.txt" \
  --entry rr --max-radius 9.3424518056034261431494609E-7 \
  > "$replay_dir/entry-k1-rr.log"

"$replay_dir/mode_cert" --prec 192 \
  --radius-file "$archive_dir/n25/radii.txt" \
  --exact-regular-similarities \
  --expect-symbols 25 --expect-negative 46 --expect-unresolved 4 \
  "$archive_dir/n25/modes.txt" > "$replay_dir/mode_cert.log"

printf 'Replay logs: %s\n' "$replay_dir"
```

The hard-coded radius is the archived `MODE_RADIUS 1` value. Other entries
must use their own exact radius from `radii.txt`; never estimate it from
displayed eigenvalues. The entry log must contain
`ENTRY_CERTIFIED k=1 conjugate_k=24 entry=rr`, and the spectrum log must
contain `MODE_CERTIFIED expected_symbols=25 expected_negative=46
expected_unresolved=4`.

This example rechecks one entry and the final spectrum. To replay the full
`n=25` certificate, repeat the entry check for modes `k=1,...,12` and entries
`rr`, `tt`, `rt_re`, `rt_im`, using the matching compressed file, entry name,
and mode radius: 48 entry checks in total. For even polygons, the Nyquist
mode `k=n/2` requires only `rr` and `tt`. The analytic Fourier, conjugacy,
reflection, and similarity identities are part of the mathematical basis
for these reductions.
