#!/usr/bin/python3

import sys

if len(sys.argv) != 4:
    sys.exit("Requires 3 arguments: <tdi> <diameter> <psi>")
TDI=float(sys.argv[1])
DIAMETER=float(sys.argv[2])
PSI=int(sys.argv[3])

THICKNESS_IN = DIAMETER * TDI

# Use equations for circular plate uniform load edges simply supported equation, working in imperial or metric
# https://www.engineersedge.com/material_science/circular_plate_uniform_load_13638.htm
v = 0.4 # Poisson's ratio
p = PSI # lbs/in^2
r = DIAMETER # Radius in inches
E = 300000 # Youngs modulus lbs/in^2, EngToolbox 0.203-0.449 Mpsi
t = THICKNESS_IN # Thickness in inches

D = E*t*t*t / (12 * (1 - v*v))
DEFLECTION_IN = (5 + v) * p * r*r*r*r / (64 * (1 + v) * D)

DEFLECTION_MM = DEFLECTION_IN * 24.5
THICKNESS_MM = THICKNESS_IN * 24.5
print("{},{},{},{:.3f},{:.3f},{:.3f},{:.3f}".format(TDI, DIAMETER, PSI, THICKNESS_IN, THICKNESS_MM, DEFLECTION_MM, DEFLECTION_IN))
