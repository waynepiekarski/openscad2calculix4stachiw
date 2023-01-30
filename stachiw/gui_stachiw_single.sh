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

echo "Run:  t/Di=${INJECT_TDI}, Di=${INJECT_DIAMETER}, psi=${INJECT_PSI}"
mkdir -p "$(dirname $0)/../temp-stachiw"
TEMP="$(dirname $0)/../temp-stachiw/temp-tdi${INJECT_TDI}-di${INJECT_DIAMETER}-psi${INJECT_PSI}.scad"
echo "inject_tdi = ${INJECT_TDI};" > "${TEMP}"
echo "inject_diameter = ${INJECT_DIAMETER};" >> "${TEMP}"
echo "inject_psi = ${INJECT_PSI};" >> "${TEMP}"
cat acrylic-stachiw.scad >> "${TEMP}"
$(dirname $0)/../gui/OpenSCAD2CalculiX "${TEMP}"
