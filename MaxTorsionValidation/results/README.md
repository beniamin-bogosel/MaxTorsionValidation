# Certificate archives and distribution

The full historical results remain in this directory locally. Their total
size is approximately 8.5 GB. Git includes this guide and the two compact
summaries below; generated archives, compressed candidate fields, private
builds, and historical logs are excluded by the root `.gitignore`.

- [Mesh m = 64 summary](extended_m64_summary.md)
- [Mesh m = 128 summary](extended_m128_summary.md)
- [Section 8 reproduction commands](../docs/REPRODUCING_SECTION_8.md)

Links from the summaries and the root review into archive directories refer
to the local or separately distributed data bundle. A source-only Git clone
does not contain those targets. No public archive URL or DOI has been
assigned in this repository.

## Regenerate a certificate without downloading archives

The source distribution is sufficient to generate new candidates and run
the complete certification chain. Start with the pentagon at `m=32` using
[the small-case command](../docs/REPRODUCING_SECTION_8.md#start-with-one-small-certificate).
The same guide gives commands for every reported table. Each run creates
its own candidate fields, residual bounds, entry-containment logs and final
sign certificate. The large historical archives are needed only to replay
the particular candidates used in the reported runs; they are not needed
to regenerate a certificate from the source code.

## Archives underlying Section 8

Paths below are relative to this directory. Preserve the complete named
archives when preparing the separate data deposit, including manifests,
source copies, compressed inputs, entry logs, reference centers, radii,
final mode logs, checksums, and execution notes.

| Mesh and polygons | Archive |
| --- | --- |
| m = 32, n = 3–10, original source/log record | `n3_n10_certificate_archive/` |
| m = 32, n = 3–10, later run with replay inputs | `review_2026_09_08_certificate_archive/` |
| m = 64, n = 11 | `extended_m64_n11/` |
| m = 64, n = 12 | `extended_m64_n12_n16/` (use completed n12) |
| m = 64, n = 13–18 | `extended_m64_fourcores_n13_n21/` |
| m = 64, n = 19–20, inconclusive sign tests | `extended_m64_n19_n20_flux1e9/` |
| m = 128, n = 19–21 | `extended_m128_fast_n19_onward/` |
| m = 128, n = 22–24 | `extended_m128_rotations_n22_n30/` |
| m = 128, n = 25 | `extended_m128_rotations_n25/` |
| Subsequent validity review and n21 replay provenance | `validity_review_20260909/` |

Some names retain the originally requested sweep endpoints; those names do
not establish which polygons completed. Consult each polygon's `RESULT.json`,
entry logs, and final `mode_cert.log`. A sweep's `FINISHED` marker also permits
an inconclusive sign test. The original small `n3_n10_certificate_archive/`
does not contain the compressed PDE candidates; the later review archive does.

The original checksum for
`extended_m128_fast_n19_onward/n21/inputs/k6_rr.txt.gz` differs from the
present file. The historical checksum was preserved; a later independent
full entry replay at the original center and radius is recorded in
`validity_review_20260909/n21-replay-provenance.json` and
`entry-replay-n21-k6-rr.log`. See [the review](../../REVIEW.md).

## Optional distribution of historical data

Copy or archive the selected directories without editing their contents.
Keep the relative paths in this guide so the summary links remain useful
when the bundle is extracted into `MaxTorsionValidation/results/`. Publish
the data separately and add its persistent download URL here once available.
Do not force-add the entire local results tree to the source repository.

Manifests and source snapshots retain the historical `code/` paths. Archive
relative `SHA256SUMS` files apply within their original archive directories;
the old top-level `results/SHA256SUMS` is an earlier release record, not a
current source manifest. The folder rename does not change any archive bytes.

New reproduction commands must use new output directories. A convenient
local convention is `MaxTorsionValidation/results/section8-new/` with a
separate new child directory for each mesh/range. See the reproduction guide
for commands and the distinction between regenerating and replaying a proof.
