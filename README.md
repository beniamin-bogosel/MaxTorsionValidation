# MaxTorsionValidation

Validated computations for local maximality of regular polygons for
torsional rigidity. The recorded certificates support the manuscript's
result for `n = 5,…,25`; `n = 3,4` are consistency checks. See
[Section 8 reproduction](MaxTorsionValidation/docs/REPRODUCING_SECTION_8.md)
for every mesh/range, expected outcome, and archive replay instructions.

The maintained manuscript and certificate concern the state `-Δu = 1`,
the energy `J = ½∫u`, and the scale-invariant objective `F = J/A²`, where
`A` is polygonal area. Conventional torsional rigidity is `2J`, so its
normalized Hessian has twice the reported eigenvalues and the same signs.
Vertices are counterclockwise, have circumradius one, and use interleaved
coordinates `(x0,y0,x1,y1,…)`.

The method follows [Bogosel–Bucur, arXiv:2406.11575](https://arxiv.org/abs/2406.11575):
vertex Hessian, Fourier reduction, continuous error bounds, validated
algebra, and a final sign test. That reference proves a Dirichlet-eigenvalue
result. This folder develops the torsion counterpart, with residual flux
bounds and a second-variation identity.

| Contents | Role |
| --- | --- |
| [Computing instructions](MaxTorsionValidation/README.md) | Maintained FreeFEM and FLINT/Arb workflow |
| [Section 8 commands](MaxTorsionValidation/docs/REPRODUCING_SECTION_8.md) | Reproduce all published certificate tables through n = 25 |
| [Three-solve rotation code](MaxTorsionValidation/freefem/ROTATIONS.md) | Optimized floating Hessian and rotation-cached certificate candidates |
| [Symbolic checks](MaxTorsionValidation/symbolic/README.md) | Exact algebraic regression checks |
| [Review](REVIEW.md) | Corrections, validation evidence, and remaining limits |
| [Results and archives](MaxTorsionValidation/results/README.md) | Compact summaries in Git; complete archives distributed separately |
| [Original supplied programs](Code/README.md) | Researcher's FreeFEM prototypes, preserved separately |
| [Prompt history](PromptHistory.md) | Available conversation and recorded model versions |
| [Provenance](PROVENANCE.md) | Origins, AI assistance, and licensing status |
| [Publishing guide](MaxTorsionValidation/docs/PUBLISHING.md) | Source distribution, exclusions, and Git staging commands |

The manuscript is maintained locally at
`theory/torsional_rigidity_regular_polygon.tex` and supplied separately.
`theory/`, `Theory/`, `Unused/`, reference PDFs in `Papers/`, binaries,
caches, and the large generated archives are excluded from Git.
The folder previously called `code/` is now `MaxTorsionValidation/`.

## Setup and reproduction

Use Linux (or WSL) for the certificate sweeps: the Python drivers use Linux
CPU-affinity APIs. Install a C11 compiler, make, FLINT 3 with Arb headers,
FreeFEM with the `lapack` plugin and `macro_ddm.idp`, Python 3, Bash, gzip,
and the usual GNU core utilities. Recorded runs used FreeFEM 4.15,
FLINT 3.0.1, and GCC 13.3.

From the repository root:

```sh
python3 -m venv .venv
. .venv/bin/activate
python3 -m pip install -r MaxTorsionValidation/requirements.txt
make -B -C MaxTorsionValidation/flint all
```

Set `FREEFEM_BIN` if FreeFEM is outside `PATH`. The build command forces
fresh local executables; the Python sweep drivers additionally compile
private verifiers from their archived sources. Optional tests are
`make -C MaxTorsionValidation/flint test`.

For the first Section 8 table (`n=3,…,10`, `m=32`):

```sh
CERT_ARCHIVE_DIR=MaxTorsionValidation/results/section8-new-m32 \
  bash MaxTorsionValidation/freefem/run_regular_certificates.sh 3 10
```

Every archive path must be new. The
[full reproduction guide](MaxTorsionValidation/docs/REPRODUCING_SECTION_8.md)
adds `m=64`, `n=11,…,20`, and `m=128`, `n=19,…,25`. The sign proof requires
`2n−4` negative eigenvalues and four analytically established similarity
zeros. It does not assume the conjectured broken regularity or an observed
convergence order.

The recorded finer-mesh runs extend certification through `n=18` at
`m=64` and through `n=25` at `m=128`; see the
[m=64 results](MaxTorsionValidation/results/extended_m64_summary.md) and
[m=128 results](MaxTorsionValidation/results/extended_m128_summary.md). The optimized
continuation includes the separately requested `n=25` certificate and stops there.

## Formula-to-code map

Equation labels below are stable LaTeX source labels. Mathematical names
refer to exact functions; executable arrays contain candidates whose errors
are subsequently bounded.

| Manuscript quantity / equation label | Maintained implementation |
| --- | --- |
| `u`, `J`, `A`, `F` (`eq:torsion-state`, `eq:J`, `eq:F`) | `u`, `J`, `A`, `HF` in `torsion_hessian.edp` |
| `U_i`, directional `u_q = Σ q_i·U_i` (`eq:material`) | `Ux`, `Uy` in benchmark; `uq`, `ur` in lifting |
| `D²J`, `D²F` (`eq:HJ`, `eq:HF-general`) | `HJ`, `HF` in benchmark |
| `B_k = [[α_k,iγ_k],[-iγ_k,β_k]]` (`eq:symbol-entries`) | `MODE_DATA k a d Re(b) Im(b)`; the off-diagonal real and imaginary parts are `0` and `γ_k` |
| Normalized real Fourier directions (`eq:real-pairing-symbol`) | `qa`, `ra`: 0 radial / 1 tangential; `qp`, `rp`: 0 cosine / 1 sine |
| Pullback matrices (`eq:Aq-pullback`, `eq:Aqr-pullback`) | `coefficients()` in `second_lifting_cert.c` |
| Exact discrete lifting `z_h` (`eq:discrete-differentiated-systems`) | candidate `zh`; `Z^h_{qr}` is its continuous lifting |
| Residual norms divided by `√α_lower`; total algebraic bounds `ε_0,ε_q,ε_r` (`eq:algebraic-transfer`) | `ae0,aeq,aer,aez`; `eps0,epsq,epsr` |
| Form norms `C_q,C_r,C_{qr}` (`eq:Cqr`) | `opq,opr,mqr`; historical output field `M_qr` means `C_{qr}` |
| Continuous bounds (`eq:combined-deltas`) | `d0tot,dqtot,drtot,etatot` |
| Corrected center and full radius (`eq:corrected-F-center`, `eq:complete-F-radius`) | `fcenter,ferror`; `ENTRY_CERTIFIED` proves containment |
| Eigenvalues and symmetry kernels (`eq:mode-eigs`, Proposition `prop:modes`) | `mode_cert.c`; exact-similarity option requires the analytic regular-polygon hypotheses |

The auxiliary sector formulas and Gram comparison explain the Hessian's
structure. The working certificate encloses real Fourier pairings directly;
it does not separately evaluate `Δ_n` or the normalized Gram variables.
