#!/bin/bash

echo "t/di,diam_in,psi,thick_in,thick_mm,deflect_mm,deflect_in" > stachiw-results.csv

do_test() {
    TDI="$1"
    DIAMETER="$2"
    PSI="$3"
    if [[ "${PSI}" == "0" ]]; then
	echo "${TDI},${DIAMETER},${PSI},0,0,0,0" >> stachiw-results.csv
    else
	echo "$(dirname $0)/run_stachiw_single.sh" "${TDI}" "${DIAMETER}" "${PSI}"
    fi
}

for DIAMETER in "1.5"; do
    # Execute each t/Di test over the approximate range it has in Figure 7.12 before breaking
    # The CalculiX results tend to be linear and not curved like in Stachiw's experiment
    TDI="0.156"; for PSI in {0..2000..100};   do do_test "${TDI}" "${DIAMETER}" "${PSI}"; done
    TDI="0.228"; for PSI in {0..4000..200};   do do_test "${TDI}" "${DIAMETER}" "${PSI}"; done
    TDI="0.331"; for PSI in {0..8000..1000};  do do_test "${TDI}" "${DIAMETER}" "${PSI}"; done
    TDI="0.407"; for PSI in {0..14000..1000}; do do_test "${TDI}" "${DIAMETER}" "${PSI}"; done
    TDI="0.489"; for PSI in {0..20000..2000}; do do_test "${TDI}" "${DIAMETER}" "${PSI}"; done
    TDI="0.559"; for PSI in {0..24000..2000}; do do_test "${TDI}" "${DIAMETER}" "${PSI}"; done
    TDI="0.655"; for PSI in {0..24000..2000}; do do_test "${TDI}" "${DIAMETER}" "${PSI}"; done
# Execute everything in parallel, since only one line per process is written there should be no corruption due to race conditions
done | time parallel --verbose

# Sort everything in-place to get consistent output ordering
sort --field-separator=',' --numeric -k1 -k2 -k3 --output=stachiw-results.csv stachiw-results.csv
