#!/bin/bash

# Expect 0.1"=2.54mm displacement from Stachiw figure 7.12 for 3.33" disc
# Calculix returns 2.64mm=0.104" which is almost exactly the same
INJECT_TDI="0.433"
INJECT_DIAMETER="3.33"
INJECT_PSI="8000"
if [[ "$3" != "" ]]; then
    INJECT_TDI="$1"
    INJECT_DIAMETER="$2"
    INJECT_PSI="$3"
fi

THICKNESS_IN="$(echo "scale=3; ${INJECT_TDI}*${INJECT_DIAMETER}" | bc -l)"
THICKNESS_MM="$(echo "scale=3; ${THICKNESS_IN}*25.4" | bc -l)"

echo "Run:  t/Di=${INJECT_TDI}, Di=${INJECT_DIAMETER}, psi=${INJECT_PSI}, thick_in=${THICKNESS_IN}, thick_mm=${THICKNESS_MM}"
mkdir -p "$(dirname $0)/../temp-stachiw"
TEMP="$(dirname $0)/../temp-stachiw/temp-tdi${INJECT_TDI}-di${INJECT_DIAMETER}-psi${INJECT_PSI}.scad"
echo "inject_tdi = ${INJECT_TDI};" > "${TEMP}"
echo "inject_diameter = ${INJECT_DIAMETER};" >> "${TEMP}"
echo "inject_psi = ${INJECT_PSI};" >> "${TEMP}"
cat acrylic-stachiw.scad >> "${TEMP}"
$(dirname $0)/../core/os2cx "${TEMP}" > "${TEMP}.stdout"
# Verify that the units are always millimeters because it might be different and we should fail then
DEFLECT_MM="$(grep "Measurement name=max_deflection_result" "${TEMP}.stdout" | grep " unit=mm" | tr '=' ' ' | awk '{ print $5 }')"
if [[ "${DEFLECT_MM}" == "" ]]; then
    echo "Error! No deflection in millimeters returned for t/Di=${INJECT_TDI}, Di=${INJECT_DIAMETER}, psi=${INJECT_PSI}"
    grep "Measurement name=max_deflection_result" "${TEMP}.stdout"
    exit 1
fi
DEFLECT_IN="$(echo "scale=3; ${DEFLECT_MM}/25.4" | bc -l)"
echo "Done: t/Di=${INJECT_TDI}, Di=${INJECT_DIAMETER}, psi=${INJECT_PSI}, thick_in=${THICKNESS_IN}, thick_mm=${THICKNESS_MM} --> deflect_mm=${DEFLECT_MM}, deflect_in=${DEFLECT_IN}"
echo "${INJECT_TDI},${INJECT_DIAMETER},${INJECT_PSI},${THICKNESS_IN},${THICKNESS_MM},${DEFLECT_MM},${DEFLECT_IN}" >> stachiw-results.csv
