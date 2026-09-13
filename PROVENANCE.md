# Provenance and distribution

`Code/` contains the researcher's original supplied FreeFEM programs.
`MaxTorsionValidation/` contains the maintained torsional-rigidity benchmark,
residual verifiers, certificate drivers, symbolic checks, and documentation
developed in this project. [PromptHistory.md](PromptHistory.md) records the
available user/assistant thread and the model identifiers recorded for it.
The history is a development record: earlier conclusions may have been
qualified by the later review.

The manuscript follows the polygonal Hessian and validated-computing strategy
of Bogosel and Bucur, with torsion shape derivatives and residual estimates
described in the manuscript. Mathematical references are given in the main
README and the separately maintained manuscript bibliography.

No project-wide software license has been selected. No blanket license is
applied to the original supplied programs, research manuscript, third-party
papers, or dependencies by this packaging change. The copyright holders
should supply the intended license and author/citation metadata before a
licensed public release. FreeFEM, FLINT/Arb, Python packages, and reference
papers retain their own terms.

`theory/` (or `Theory/`), `Unused/`, and `Papers/` are excluded from the Git
source distribution. They remain available locally. The approximately
8.5 GB of historical results also remains local; see the
[archive distribution guide](MaxTorsionValidation/results/README.md).

The September 2026 packaging change renames `code/` to
`MaxTorsionValidation/` and updates active paths. Archived source snapshots,
manifests, candidates, logs, and checksums retain their original contents,
including recorded `code/` paths. Historical checksum files describe their
original snapshots and are not checksums of the reorganized current source.
No solver, certificate, or mathematical test was rerun for this change.

The subsequent [final package recheck](PACKAGE_RECHECK.md) records the
builds, numerical checks, and archive audits performed after reorganization.
