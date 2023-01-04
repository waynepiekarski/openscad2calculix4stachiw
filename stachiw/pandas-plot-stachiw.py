#!/usr/bin/python3

import sys

INPUT_ARGS = sys.argv[1:]

if len(INPUT_ARGS) != 1:
    print("Need input comma separated text file")
    sys.exit(1)
infile = INPUT_ARGS[0]
print ("Input file {}".format(infile))

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Dump out any tables without any wrapping
# pd.set_option("display.max_rows", None, "display.max_columns", None, "display.width", 10000, "display.max_colwidth", 10000)

table = pd.read_csv(infile, comment='#', header=0, engine='python', skiprows=0)

# Generate millimeters values since Stachiw's experiments were done in inches
print("** Processing metric conversions")
table['diam_mm'] = table['diam_in'] * 25.4
table.insert(table.columns.get_loc('diam_in')+1, 'diam_mm', table.pop('diam_mm'))

# Dump out all the column header names
print(table.columns.tolist())

# The output was generated in parallel so need to sort to make it easier to read
table.sort_values(by=['t/di', 'diam_in', 'psi'], inplace=True)

# Dump out the entire table for debugging
print("** Entire table")
print(table)

# Disable scientific notation which sometimes occurs on the Y-axis when the range doesn't vary much
plt.rcParams["axes.formatter.useoffset"] = False

# Plot similar to Stachiw Figure 7.12 with X=deflection and Y=pressure with lines for each T/di
print("** Plotting graphs")

fig, ax = plt.subplots()
for key, grp in table.groupby(['t/di']):
    ax = grp.plot(ax=ax, kind='line', x='deflect_in', y='psi', label=key, style='.-')
# Set graph axes to match Figure 7.12
plt.xlim([0,1.02])
plt.ylim([0,28500])
plt.xticks(np.arange(0, 1.1, 0.1))
plt.yticks(np.arange(0, 29000, 4000))

# Fix up the legend values to only have 3 digits instead of long floating noise
leg = ax.get_legend()
for lbl in leg.get_texts():
    label_num = lbl.get_text()
    new_text = f'X = {float(label_num):,.3f}'
    lbl.set_text(new_text)

plt.show()
