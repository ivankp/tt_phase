#!/bin/bash

mkdir -p dat fit

for f in $(ls | grep '.json$'); do
  if [ "$f" -nt "dat" ] || [ -z "$(ls -A dat)" ]; then
    echo $f
    ../bin/vars $f -o dat/$(basename $f .json) \
      -b 200 250 300 350 400 450 500 550
  fi
done

for f in $(ls dat | grep '.dat$'); do
  for phi in '' 0; do
    ofname=fit/$(basename $f .dat)
    if [ -n "$phi" ]; then
      ofname+="_phi$phi"
      phi_arg="--phi=$phi"
    fi
    ofname+=".json"
    if [ ! -f "$ofname" ] || [ "$f" -nt "$ofname" ]; then
      ../bin/fit dat/$f -o $ofname \
        --print-level=-1 -r0.8 $phi_arg
    fi
  done
done

