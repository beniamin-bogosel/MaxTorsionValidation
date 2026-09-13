#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd "$(dirname "$0")/../.." && pwd)
ff_bin=${FREEFEM_BIN:-FreeFem++}
n_first=${1:-3}
n_last=${2:-10}
mesh_m=${CERT_MESH_M:-32}
verbose=${CERT_VERBOSE:-0}
archive_dir=${CERT_ARCHIVE_DIR:-}
cert_prec=192

if ((n_first < 3 || n_last > 10 || n_first > n_last)); then
  echo "usage: $0 [n_first>=3] [n_last<=10]" >&2
  exit 2
fi
if ((mesh_m != 32)); then
  echo "calibrated certificate radii currently require CERT_MESH_M=32" >&2
  exit 2
fi

work_dir=$(mktemp -d)
trap 'rm -rf "$work_dir"' EXIT
make -C "$repo_dir/MaxTorsionValidation/flint" all

if [[ -n "$archive_dir" ]]; then
  if [[ -e "$archive_dir" ]]; then
    echo "CERT_ARCHIVE_DIR must name a new path: $archive_dir" >&2
    exit 2
  fi
  mkdir -p "$archive_dir"
  {
    printf 'Regular-polygon torsion certificate archive\n'
    printf 'created_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    printf 'n_first=%s\nn_last=%s\nmesh_m=%s\n' "$n_first" "$n_last" "$mesh_m"
    printf 'pde_precision_bits=%s\nmode_precision_bits=%s\n' "$cert_prec" "$cert_prec"
    printf 'geom_radius=1e-13\ndirection_radius=1e-13\n'
    printf '\nFreeFEM version output:\n'
    "$ff_bin" -v 0 -nw 2>&1 || true
    printf '\nC compiler:\n'
    "${CC:-cc}" --version 2>&1 | sed -n '1p' || true
    printf '\nFLINT pkg-config version:\n'
    pkg-config --modversion flint 2>&1 || true
    printf '\nSource SHA256:\n'
    sha256sum \
      "$repo_dir/MaxTorsionValidation/freefem/torsion_hessian.edp" \
      "$repo_dir/MaxTorsionValidation/freefem/second_variation_lifting.edp" \
      "$repo_dir/MaxTorsionValidation/freefem/run_regular_certificates.sh" \
      "$repo_dir/MaxTorsionValidation/flint/mode_cert.c" \
      "$repo_dir/MaxTorsionValidation/flint/second_lifting_cert.c" \
      "$repo_dir/MaxTorsionValidation/flint/Makefile" \
      "$repo_dir/MaxTorsionValidation/symbolic/verify_hessian_reduction.py" \
      "$repo_dir/MaxTorsionValidation/symbolic/verify_pullback_coefficients.py"
  } >"$archive_dir/MANIFEST.txt"
  mkdir -p "$archive_dir/sources" "$archive_dir/inputs"
  cp \
    "$repo_dir/MaxTorsionValidation/freefem/torsion_hessian.edp" \
    "$repo_dir/MaxTorsionValidation/freefem/second_variation_lifting.edp" \
    "$repo_dir/MaxTorsionValidation/freefem/run_regular_certificates.sh" \
    "$repo_dir/MaxTorsionValidation/flint/mode_cert.c" \
    "$repo_dir/MaxTorsionValidation/flint/second_lifting_cert.c" \
    "$repo_dir/MaxTorsionValidation/flint/Makefile" \
    "$repo_dir/MaxTorsionValidation/symbolic/verify_hessian_reduction.py" \
    "$repo_dir/MaxTorsionValidation/symbolic/verify_pullback_coefficients.py" \
    "$archive_dir/sources/"
fi

mode_radius()
{
  case "$1:$2" in
    3:1)  echo 1.32e-3 ;;
    4:1)  echo 2.60e-4 ;;
    4:2)  echo 5.70e-4 ;;
    5:1)  echo 1.80e-4 ;;
    5:2)  echo 4.80e-4 ;;
    6:1)  echo 1.30e-4 ;;
    6:2)  echo 3.30e-4 ;;
    6:3)  echo 4.10e-4 ;;
    7:1)  echo 9.60e-5 ;;
    7:2)  echo 2.80e-4 ;;
    7:3)  echo 5.20e-4 ;;
    8:1)  echo 7.50e-5 ;;
    8:2)  echo 1.85e-4 ;;
    8:3)  echo 4.65e-4 ;;
    8:4)  echo 4.80e-4 ;;
    9:1)  echo 6.40e-5 ;;
    9:2)  echo 1.90e-4 ;;
    9:3)  echo 4.10e-4 ;;
    9:4)  echo 6.60e-4 ;;
    10:1) echo 5.50e-5 ;;
    10:2) echo 1.60e-4 ;;
    10:3) echo 4.00e-4 ;;
    10:4) echo 6.50e-4 ;;
    10:5) echo 6.20e-4 ;;
    *) echo "no calibrated radius for n=$1 k=$2" >&2; return 2 ;;
  esac
}

for ((n=n_first; n<=n_last; ++n)); do
  mode_file="$work_dir/modes_n${n}.txt"
  radius_file="$work_dir/radii_n${n}.txt"
  echo "CERTIFICATE_START n=$n m=$mesh_m"
  "$ff_bin" -v 0 -nw "$repo_dir/MaxTorsionValidation/freefem/torsion_hessian.edp" \
    -n "$n" -m "$mesh_m" >"$mode_file"

  printf 'MODE_RADIUS 0 0\n' >"$radius_file"
  for ((k=1; k<n; ++k)); do
    representative=$k
    if ((representative > n/2)); then representative=$((n-k)); fi
    radius=$(mode_radius "$n" "$representative")
    printf 'MODE_RADIUS %d %s\n' "$k" "$radius" >>"$radius_file"
  done

  for ((k=1; k<=n/2; ++k)); do
    radius=$(mode_radius "$n" "$k")
    if ((n % 2 == 0 && k == n/2)); then
      specs=("rr 0 0 0 0" "tt 1 0 1 0")
    else
      specs=("rr 0 0 0 0" "tt 1 0 1 0" \
             "rt_re 0 0 1 0" "rt_im 0 0 1 1")
    fi
    for spec in "${specs[@]}"; do
      read -r name qa qp ra rp <<<"$spec"
      export_file="$work_dir/n${n}_k${k}_${name}.txt"
      "$ff_bin" -v 0 -nw "$repo_dir/MaxTorsionValidation/freefem/second_variation_lifting.edp" \
        -n "$n" -m "$mesh_m" -k "$k" \
        -qa "$qa" -qp "$qp" -ra "$ra" -rp "$rp" \
        -export-second "$export_file" >/dev/null
      cert_output=$("$repo_dir/MaxTorsionValidation/flint/second_lifting_cert" \
        "$export_file" --prec "$cert_prec" --geom-radius 1e-13 \
        --direction-radius 1e-13 --check-regular-inputs \
        --reference "$mode_file" --entry "$name" --max-radius "$radius")
      if [[ -n "$archive_dir" ]]; then
        printf '%s\n' "$cert_output" >"$archive_dir/n${n}_k${k}_${name}.log"
        gzip -n -c "$export_file" >"$archive_dir/inputs/n${n}_k${k}_${name}.txt.gz"
      fi
      certified_markers=0
      if ((verbose)); then
        printf '%s\n' "$cert_output"
      else
        while IFS= read -r line; do
          case "$line" in
            exact_mesh_topology=*|exact_regular_fan_inputs=*|ENTRY_CERTIFIED*)
              echo "$line"
              ;;
            esac
        done <<<"$cert_output"
      fi
      while IFS= read -r line; do
        case "$line" in
          ENTRY_CERTIFIED*) certified_markers=$((certified_markers+1)) ;;
        esac
      done <<<"$cert_output"
      if ((certified_markers != 1)); then
        echo "expected exactly one ENTRY_CERTIFIED marker for n=$n k=$k entry=$name" >&2
        exit 3
      fi
    done
  done

  if [[ -n "$archive_dir" ]]; then
    cp "$mode_file" "$archive_dir/modes_n${n}.txt"
    cp "$radius_file" "$archive_dir/radii_n${n}.txt"
  fi
  expected_negative=$((2*n-4))
  # Match the precision used for reference-ball containment exactly.
  mode_output=$("$repo_dir/MaxTorsionValidation/flint/mode_cert" --prec "$cert_prec" --radius-file "$radius_file" \
    --exact-regular-similarities --expect-symbols "$n" \
    --expect-negative "$expected_negative" --expect-unresolved 4 "$mode_file")
  printf '%s\n' "$mode_output"
  if [[ -n "$archive_dir" ]]; then
    printf '%s\n' "$mode_output" >"$archive_dir/mode_cert_n${n}.log"
  fi
  echo "REGULAR_N_CERTIFIED n=$n negative=$expected_negative zero=4 m=$mesh_m"
done

if [[ -n "$archive_dir" ]]; then
  (cd "$archive_dir" && find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum >SHA256SUMS)
  printf 'COMPLETE n_first=%s n_last=%s mesh_m=%s\n' \
    "$n_first" "$n_last" "$mesh_m" >"$archive_dir/COMPLETE"
fi
