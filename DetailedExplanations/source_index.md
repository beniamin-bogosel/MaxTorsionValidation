# Source locations for Lecture 3

Line numbers refer to the source hashes in [sources.json](sources.json). The lecture explains every function listed below; named FreeFEM problems are included as entry points.

## [MaxTorsionValidation/flint/second_lifting_cert.c](../MaxTorsionValidation/flint/second_lifting_cert.c)

| Function / problem | Line |
| --- | ---: |
| `die` | 24 |
| `cmp_edge` | 26 |
| `cmp_tri_key` | 33 |
| `dsu_root` | 41 |
| `dsu_join` | 47 |
| `certify_mesh_topology` | 56 |
| `read_ball` | 100 |
| `read_field` | 107 |
| `gradient` | 114 |
| `add_affine_square` | 120 |
| `affine_integral` | 129 |
| `assemble_triangle_geometry` | 136 |
| `symmetric_norm` | 156 |
| `coefficients` | 166 |
| `state_error` | 211 |
| `canonical_gid` | 223 |
| `canonical_tri_key` | 232 |
| `validate_regular_inputs` | 240 |
| `interior_residual_norm` | 349 |
| `read_mode_reference` | 357 |
| `main` | 373 |

## [MaxTorsionValidation/flint/mode_cert.c](../MaxTorsionValidation/flint/mode_cert.c)

| Function / problem | Line |
| --- | ---: |
| `usage` | 18 |
| `print_ball` | 24 |
| `main` | 29 |

## [MaxTorsionValidation/flint/majorant_cert.c](../MaxTorsionValidation/flint/majorant_cert.c)

| Function / problem | Line |
| --- | ---: |
| `die` | 26 |
| `read_arb` | 32 |
| `read_field` | 40 |
| `grad_field` | 51 |
| `add_affine_square` | 62 |
| `energy_norm` | 78 |
| `state_majorant` | 92 |
| `material_majorant` | 119 |
| `print_ball` | 159 |
| `main` | 164 |

## [MaxTorsionValidation/flint/residual_cert.c](../MaxTorsionValidation/flint/residual_cert.c)

| Function / problem | Line |
| --- | ---: |
| `read_arb` | 19 |
| `main` | 25 |

## [MaxTorsionValidation/flint/second_variation_cert.c](../MaxTorsionValidation/flint/second_variation_cert.c)

| Function / problem | Line |
| --- | ---: |
| `die` | 14 |
| `set_nonnegative` | 20 |
| `main` | 26 |

## [MaxTorsionValidation/freefem/certify_with_rotations.py](../MaxTorsionValidation/freefem/certify_with_rotations.py)

| Function / problem | Line |
| --- | ---: |
| `run` | 25 |
| `ball` | 35 |
| `main` | 42 |
| `evaluate` | 131 |
| `verify` | 183 |

## [MaxTorsionValidation/freefem/rotation_cache.py](../MaxTorsionValidation/freefem/rotation_cache.py)

| Function / problem | Line |
| --- | ---: |
| `prepare` | 9 |
| `direction` | 76 |

## [MaxTorsionValidation/freefem/certificate_build.py](../MaxTorsionValidation/freefem/certificate_build.py)

| Function / problem | Line |
| --- | ---: |
| `build_verifiers` | 7 |

## [MaxTorsionValidation/freefem/second_variation_lifting_rotations.edp](../MaxTorsionValidation/freefem/second_variation_lifting_rotations.edp)

| Function / problem | Line |
| --- | ---: |
| `state` | 72 |
| `firstQ` | 74 |
| `firstR` | 77 |
| `lifting` | 80 |
| `fitCurl` | 121 |

## [MaxTorsionValidation/freefem/torsion_hessian_rotations.edp](../MaxTorsionValidation/freefem/torsion_hessian_rotations.edp)

| Function / problem | Line |
| --- | ---: |
| `torsion` | 48 |
| `materialX` | 67 |
| `materialY` | 73 |

## [MaxTorsionValidation/freefem/test_extended_certificates.py](../MaxTorsionValidation/freefem/test_extended_certificates.py)

| Function / problem | Line |
| --- | ---: |
| `run` | 25 |
| `ball` | 35 |
| `main` | 42 |
| `evaluate` | 117 |
| `verify` | 167 |

## [MaxTorsionValidation/freefem/summarize_extended_certificates.py](../MaxTorsionValidation/freefem/summarize_extended_certificates.py)

| Function / problem | Line |
| --- | ---: |
| `audit` | 11 |
| `main` | 93 |

