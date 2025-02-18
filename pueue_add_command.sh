#!/usr/bin/env bash

for start in $(seq 0 17); do
  end=$((start + 7))

  output="twitter_"
  for idx in $(seq $start $end); do
    output="${output}${idx}_"
  done
  output="${output%_}.info"

  inputs=""
  for idx in $(seq $start $end); do
    inputs="$inputs /root/shared/twitter/twitter_split_${idx}.csv"
  done

  pueue add -g trace_info_8 ./trace_info.out -o "$output" $inputs

done
