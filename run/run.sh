#!/bin/bash

mkdir -p dat fit

for f in $(ls | grep '.json$'); do
  base=$(basename $f .json)
  if [ -z "$(find dat -name "${base}*")" ] || \
     [ -n "$(find dat -name "${base}*" -not -newer $f)" ] || \
     [ -n "$(find dat -name "${base}*" -not -newer ../bin/vars)" ]
  then
    echo $f
    ../bin/vars $f -o dat/$base \
      -b 200 250 300 350 400 450 500
  fi
done

for f in $(ls dat | grep '.dat$'); do
  for phi in '' 0; do
    ofname=fit/$(basename $f .dat)
    if [ -n "$phi" ]; then
      ofname+="_phi$phi"
      phi_arg="--phi=$phi"
    else
      phi_arg=''
    fi
    ofname+=".json"
    if [ ! -f "$ofname" ] || \
       [ "$ofname" -ot "dat/$f" ] || \
       [ "$ofname" -ot "../bin/fit" ]
    then
      ../bin/fit dat/$f -o $ofname --print-level=-1 -r0.8 $phi_arg
    fi
  done
done

for f in $(ls | grep '.json$'); do
  base=$(basename $f .json)
  pdf=${base}.pdf
  if [ ! -f "$pdf" ] || \
     [ -n "$(find fit -name "${base}*" -newer $pdf)" ] || \
     [ "$pdf" -ot "../bin/draw" ]
  then
    ../bin/draw $(find fit -name "${base}*" | sort) -o $pdf -y 0.2:1.4
  fi
done

