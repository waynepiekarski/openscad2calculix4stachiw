#!/bin/bash

CALCULIX="stachiw-calculix.csv"
EQUATION="stachiw-equation.csv"
echo "t/di,diam_in,psi,thick_in,thick_mm,deflect_mm,deflect_in" > "${CALCULIX}"
cp "${CALCULIX}" "${EQUATION}"

do_test() {
    TDI="$1"
    DIAMETER="$2"
    PSI="$3"
    if [[ "${PSI}" == "0" ]]; then
	echo "${TDI},${DIAMETER},${PSI},0,0,0,0" >> "${CALCULIX}"
	echo "${TDI},${DIAMETER},${PSI},0,0,0,0" >> "${EQUATION}"
    else
        # Queue up CalculiX command for parallel run later
        echo "$(dirname $0)/run_stachiw_single.sh" "${TDI}" "${DIAMETER}" "${PSI}"
        # Perform equation immediately since it is very fast
        "$(dirname $0)/run_equation_single.py" "${TDI}" "${DIAMETER}" "${PSI}" >> "${EQUATION}"
    fi
}

# Execute each t/Di test over the range of the Figure 7.12 pressure range
# CalculiX and equation results are always perfectly linear and not curved like Stachiw's experiment,
# so there is no need for detailed fine resolution. So use less steps to make the calculations faster
# but still see the linear relationship.
for PSI in "0" "1000" "2000" "4000" "8000" "16000" "28000"; do
    DIAMETER="1.5"
    for TDI in "0.082" "0.156" "0.228" "0.331" "0.407" "0.489" "0.559" "0.655"; do
	do_test "${TDI}" "${DIAMETER}" "${PSI}"
    done
    DIAMETER="3.33"
    for TDI in "0.036" "0.102" "0.182" "0.251" "0.338" "0.433" "0.600"; do
	do_test "${TDI}" "${DIAMETER}" "${PSI}"
    done
    DIAMETER="4.0"
    for TDI in "0.058" "0.110" "0.241" "0.498"; do
	do_test "${TDI}" "${DIAMETER}" "${PSI}"
    done
    # 1.5cm window ==> 0.590in, 1/2" thick acrylic, t/Di=0.847
    DIAMETER="0.590"
    for TDI in "0.847"; do
	do_test "${TDI}" "${DIAMETER}" "${PSI}"
    done
# Execute everything in parallel, since only one line per process is written there should be no corruption due to race conditions
done | time parallel --verbose

# Sort everything in-place to get consistent output ordering
sort --field-separator=',' --numeric -k1 -k2 -k3 --output="${CALCULIX}" "${CALCULIX}"
sort --field-separator=',' --numeric -k1 -k2 -k3 --output="${EQUATION}" "${EQUATION}"
