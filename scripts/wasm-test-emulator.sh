#!/usr/bin/env bash
set -euo pipefail

wasmtime=$1 source_dir=$2
binary_dir=$3 module=$4

guest_args=()
for argument in "${@:5}"; do
  case $argument in
    "$binary_dir"|"$binary_dir"/*)
      guest_path=${argument#"$binary_dir"}
      guest_args+=("${guest_path:-/}")
      ;;
    "$source_dir"|"$source_dir"/*)
      guest_args+=("/source${argument#"$source_dir"}")
      ;;
    *)
      guest_args+=("$argument")
      ;;
  esac
done

exec "$wasmtime" run "--dir=$binary_dir::/" \
  "--dir=$source_dir::/source" \
  "$module" "${guest_args[@]}"
