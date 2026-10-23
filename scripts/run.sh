#!/usr/bin/env bash
set -u

project_dir=$(cd -- "$(dirname -- "$0")/.." && pwd)
cd "$project_dir"

pids=()
cleanup() {
  if ((${#pids[@]})); then
    kill "${pids[@]}" 2>/dev/null || true
    wait "${pids[@]}" 2>/dev/null || true
  fi
}
trap cleanup EXIT INT TERM

./build/autorob_hub &
pids+=("$!")
./build/min_heap_node &
pids+=("$!")
./build/heap_sort_node &
pids+=("$!")
./build/astar_node &
pids+=("$!")

# The runtime remains in the foreground. If one required process exits, stop all.
wait -n "${pids[@]}"
status=$?
if ((status == 0)); then status=1; fi
exit "$status"
