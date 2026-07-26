#!/usr/bin/env bash
set -euo pipefail

if (( $# < 1 )); then
  echo "usage: $0 EXOFMS_SOURCE_ROOT [nprofile ...]" >&2
  exit 2
fi

exofms_root=$1
shift
if [[ ! -f "$exofms_root/src/sw_Toon_mod.f90" || ! -f "$exofms_root/src/lw_Toon_mod.f90" ]]; then
  echo "EXOFMS_SOURCE_ROOT does not contain src/sw_Toon_mod.f90 and src/lw_Toon_mod.f90" >&2
  exit 2
fi

if (( $# == 0 )); then
  profiles=(1000 10000 100000)
else
  profiles=("$@")
fi

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT
fc=${FC:-gfortran}
warmup=${EXOFMS_WARMUP:-3}
repeats=${EXOFMS_REPEATS:-10}
layers=${EXOFMS_LAYERS:-40}
ssa=${EXOFMS_SSA:-0.5}
asymmetry=${EXOFMS_ASYMMETRY:-0.5}

"$fc" -O3 -fopenmp -ffree-line-length-none -J "$build_dir" -I "$build_dir" \
  "$exofms_root/src/WENO4_mod.f90" \
  "$exofms_root/src/sw_Toon_mod.f90" \
  "$exofms_root/src/lw_Toon_mod.f90" \
  "$script_dir/benchmark_exofms_toon.f90" \
  -o "$build_dir/benchmark_exofms_toon"

for nprofile in "${profiles[@]}"; do
  "$build_dir/benchmark_exofms_toon" "$nprofile" "$layers" "$warmup" "$repeats" "$ssa" "$asymmetry"
done
