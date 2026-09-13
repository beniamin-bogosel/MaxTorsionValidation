#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd "$(dirname "$0")/../.." && pwd)
ff_bin=${FREEFEM_BIN:-FreeFem++}
work_dir=$(mktemp -d)
trap 'rm -rf "$work_dir"' EXIT
mode_file="$repo_dir/MaxTorsionValidation/flint/freefem_n5_m32_modes.txt"
entry_radius=4.9e-4

make -C "$repo_dir/MaxTorsionValidation/flint" all
for k in 1 2; do
  for spec in "rr 0 0 0 0" "tt 1 0 1 0" \
              "rt_re 0 0 1 0" "rt_im 0 0 1 1"; do
    read -r name qa qp ra rp <<<"$spec"
    export_file="$work_dir/k${k}_${name}.txt"
    "$ff_bin" -v 0 -nw "$repo_dir/MaxTorsionValidation/freefem/second_variation_lifting.edp" \
      -n 5 -m 32 -k "$k" -qa "$qa" -qp "$qp" -ra "$ra" -rp "$rp" \
      -export-second "$export_file"
    "$repo_dir/MaxTorsionValidation/flint/second_lifting_cert" "$export_file" --prec 192 \
      --geom-radius 1e-13 --direction-radius 1e-13 --check-regular-inputs \
      --reference "$mode_file" --entry "$name" --max-radius "$entry_radius"
  done
done

# Each preceding call has now proved that both its entry and the conjugate
# entry are contained in the corresponding reference ball.  The final call
# must prove exactly the expected six transverse negative modes.
"$repo_dir/MaxTorsionValidation/flint/mode_cert" --prec 192 --radius "$entry_radius" \
  --exact-regular-similarities \
  --expect-symbols 5 --expect-negative 6 --expect-unresolved 4 "$mode_file"
