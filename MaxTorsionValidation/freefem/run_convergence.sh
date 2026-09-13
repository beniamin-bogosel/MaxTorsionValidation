#!/usr/bin/env bash
set -euo pipefail

ff_bin="${FREEFEM_BIN:-FreeFem++}"
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
n="${1:-5}"

for m in 6 8 12 16 24 32; do
  "$ff_bin" -nw "$script_dir/torsion_hessian.edp" -n "$n" -m "$m" \
    | sed -n '/^SUMMARY/p;/^MODE/p'
done

