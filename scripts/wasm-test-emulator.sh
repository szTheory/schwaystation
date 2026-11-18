#!/usr/bin/env bash
set -euo pipefail
wasmtime=$1 source_dir=$2 binary_dir=$3 module=$4
guest_args=()
for argument in "${@:5}"; do
  prefix=
  if [[ $argument =~ ^([0-9]+):(.*)$ ]]; then
    prefix="${BASH_REMATCH[1]}:"
    argument=${BASH_REMATCH[2]}
  fi
  case $argument in
    "$binary_dir"|"$binary_dir"/*)
      guest_path=${argument#"$binary_dir"}
      guest_args+=("$prefix${guest_path:-/}")
      ;;
    "$source_dir"|"$source_dir"/*)
      guest_args+=("$prefix/source${argument#"$source_dir"}")
      ;;
    *)
      guest_args+=("$prefix$argument")
      ;;
  esac
done
exec "$wasmtime" run --wasi=cwd=/ "--dir=$binary_dir::/" \
  "--dir=$source_dir::/source" "$module" "${guest_args[@]}"
