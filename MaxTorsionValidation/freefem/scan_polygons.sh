#!/usr/bin/env bash
set -euo pipefail

ff_bin="${FREEFEM_BIN:-FreeFem++}"
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
m="${1:-16}"
n_first="${2:-3}"
n_last="${3:-12}"

for ((n=n_first; n<=n_last; ++n)); do
  "$ff_bin" -nw "$script_dir/torsion_hessian.edp" -n "$n" -m "$m" \
    | sed -n '/^SUMMARY/p;/^MODE k=/p'
done
