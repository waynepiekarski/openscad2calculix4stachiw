#!/bin/bash

for TDI in "0.559"; do
    for DIAMETER in "1.5"; do
	for PSI in {1000..32000..1000}; do
	    echo "$(dirname $0)/run_stachiw_single.sh" "${TDI}" "${DIAMETER}" "${PSI}"
	done
    done
done | parallel --verbose
