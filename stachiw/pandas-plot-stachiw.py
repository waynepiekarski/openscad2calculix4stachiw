#!/usr/bin/python3

import sys

CALCULIX_RESULTS = "stachiw-calculix.csv"
EQUATION_RESULTS = "stachiw-equation.csv"

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Dump out any tables without any wrapping
# pd.set_option("display.max_rows", None, "display.max_columns", None, "display.width", 10000, "display.max_colwidth", 10000)

# Read in the CalculiX results from the single CSV generated in parallel
calculix = pd.read_csv(CALCULIX_RESULTS, comment='#', header=0, engine='python', skiprows=0)

# Add more details to the t/Di labels and this also prevents Pandas using very long floating point numbers for the legend labels
calculix['label'] = calculix['diam_in'].astype(str) + "in " + calculix['t/di'].round(3).apply(str) + " calcx"
table = calculix

# Read in the simple equation results
equation = pd.read_csv(EQUATION_RESULTS, comment='#', header=0, engine='python', skiprows=0)
equation['label'] = equation['diam_in'].astype(str) + "in " + equation['t/di'].round(3).apply(str) + " simp"
table = pd.concat([table, equation])

# Import all the manually captured Stachiw Figure 7.12 results via WebPlotDigitizer https://apps.automeris.io/wpd/
import glob
FIGURE_RESULTS = glob.glob("stachiw-fig7.12-p33*.csv")
for each in FIGURE_RESULTS:
    parts = each.replace('.csv','').replace('inch','').split('-')[3].split('_')
    print("file={}, Di={}, t/Di={}".format(each, parts[0], parts[3]))
    csv = pd.read_csv(each, header=0, engine='python', names=['deflect_in', 'psi'])
    print(csv)
    csv.loc[len(csv.index)] = [0.0, 0.0] # Always add a 0 psi measurement not included in the CSV
    csv['t/di'] = float(parts[3])
    csv['diam_in'] = float(parts[0])
    csv['label'] = csv['diam_in'].astype(str) + "in " + csv['t/di'].astype(str) + " fig7.12"
    csv['thick_in'] = csv['t/di'] * csv['diam_in']
    csv['thick_mm'] = csv['thick_in'] * 25.4
    csv['deflect_mm'] = csv['deflect_in'] * 25.4
    print(csv)
    table = pd.concat([table, csv])

print("** All combined tables")
print(table)

# Generate millimeters values since Stachiw's experiments were done in inches
print("** Processing metric conversions")
table['diam_mm'] = table['diam_in'] * 25.4
table.insert(table.columns.get_loc('diam_in')+1, 'diam_mm', table.pop('diam_mm'))

# Dump out all the column header names
print(table.columns.tolist())

# The output was generated in parallel so need to sort to make it easier to read
table.sort_values(by=['t/di', 'diam_in', 'psi'], inplace=True)

# Show the labels currently used in the results
print("** Unique labels for results")
print(table['label'].unique())

# Dump out the entire table for debugging
print("** Entire table")
print(table)

# Disable scientific notation which sometimes occurs on the Y-axis when the range doesn't vary much
plt.rcParams["axes.formatter.useoffset"] = False

print("** Plotting separate graphs for {}".format(table['diam_in'].unique()))
for diameter in table['diam_in'].unique():
    # Plot similar to Stachiw Figure 7.12 with X=deflection and Y=pressure with lines for each T/di
    print("** Plotting graph for {}".format(diameter))
    ftable = table[table['label'].str.contains("{}in ".format(diameter))]
    print(ftable)

    # Vary the color palette based on the label ordering to make sure the colors match up
    colors = ['gray', 'r', 'g', 'b', 'c', 'm', 'y', '#800000', '#008000', '#000080']
    count = -1
    last = ""
    fig, ax = plt.subplots()
    fig.canvas.set_window_title("Diameter {} inches".format(diameter))
    for key, grp in ftable.groupby(['label']):
        if last != key.split()[1]:
            count += 1
            if count >= len(colors):
                count = 0
            last = key.split()[1]
        style = '.-' # https://matplotlib.org/2.1.2/api/_as_gen/matplotlib.pyplot.plot.html#matplotlib-pyplot-plot
        if key.split()[2] == 'calcx':
            style = '.--'
        elif key.split()[2] == 'simp':
            style = '.:'
        print ("Plotting key={} with color={}/{}".format(key, count, colors[count]))
        ax = grp.plot(ax=ax, kind='line', x='deflect_in', y='psi', label=key, style=style, color=colors[count])

    # Set graph axes to a subset of Figure 7.12 for more detail, this must be done here per subplot()
    if diameter == 3.33:
        plt.xlim([0,0.65])
        plt.xticks(np.arange(0, 0.65, 0.1))
    else:
        plt.xlim([0,0.45])
        plt.xticks(np.arange(0, 0.45, 0.1))
    plt.ylim([0,28500])
    plt.yticks(np.arange(0, 29000, 4000))

    # Save figure to a PDF as well as display it later
    fig.set_size_inches(11, 8.5)
    fig.savefig("plot-{}in.pdf".format(diameter), dpi=1200)
    fig.savefig("plot-{}in.png".format(diameter), dpi=150)

# Only show() once so all the windows are visible simultaneously
plt.show()
