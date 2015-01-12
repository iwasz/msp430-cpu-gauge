#!/usr/bin/env python

import numpy as np;
import math as m;
import sys
from scipy.interpolate import interp1d
import matplotlib.pyplot as plt;
import csv;

MICRO_STEPS_PER_1_STEP = 32;

# Input : sequence of readings from rotary encoder. Range of this input data must include exactly 4 steps, i.e.
# one "tooth sequence".
inData = [];

with open('capture4.csv', 'rb') as csvfile:
	reader = csv.reader(csvfile, delimiter=',');

	for row in reader:
		if len (row) > 0:
			inData.append (float (row[0]));


# Now I normalize the stream of numbers (simply by substracting the first number) so the whole data chunk starts
# from 0. So for now 0 means the angle 0

firstInData = inData[0];
inData = [val - firstInData for val in inData];

inDataX = range (0, len (inData));

# Normalize further. i.e. we want the y data be in the range of [0, 2 PI], and x data in the range of [0, 4 * MICRO_STEPS_PER_1_STEP]
# This will give us more meaningful data and insight into what is really happening in each micro-step (i.e. angle on every uStep). 
factorY = inData[-1] / (2*m.pi);
inData = [val / factorY for val in inData];

factorX = float (inDataX[-1]) / (MICRO_STEPS_PER_1_STEP * 4.0);
inDataX = [val / factorX for val in inDataX];

# Construct the reflection matrix. If you would plot our input data it should align on y = a*x straight
# line. The point is that we want to make a reflection of this data over this line to perform a correction.
lx = inDataX[-1];
ly = inData[-1]; 
llenq = m.sqrt (lx ** 2 + ly ** 2) ** 2;

reflect = np.matrix ([[lx ** 2 - ly ** 2, 2 * lx * ly], [2 * lx * ly, ly ** 2 - lx ** 2]]);
reflect = (1 / llenq) * reflect;

inDataZZ = [[x, y] for x, y in zip (inDataX, inData)];
inDataZ2 = inDataZZ * reflect;

unzipped = zip (*inDataZ2.tolist ());
ux = unzipped[0];
uy = unzipped[1];

plt.plot (inDataX, inData, '-', 
	  ux, uy, '-');

plt.legend (['input', 'reflect']);
plt.show ();

f = interp1d (ux, uy);
# Generate exactly MICRO_STEPS_PER_1_STEP * 4 points
ux2 = np.linspace(0, ux[-1], MICRO_STEPS_PER_1_STEP * 4);

'''
x = [];
y = [];

with open('capture2.csv', 'rb') as csvfile:
	reader = csv.reader(csvfile, delimiter=',');

	for row in reader:
		x.append (float (row[0]));
		y.append (float (row[1]));


factorY = y[-1] / (2*m.pi);
y = [val / factorY for val in y];

factorX = x[-1] / 512;
x = [val / factorX for val in x];

print (x);
print (y);
sys.exit (0);

f = interp1d(x, y);
x2 = np.linspace(0, x[-1], 512);

#plt.plot (x, y, 'o', x2, f(x2), '-');
#plt.legend (['original', 'interpolated']);
#plt.show ();
'''
# --------------------

winding1 = [];
winding2 = [];

winding1O = [];
winding2O = [];

g = np.linspace(0, 2*m.pi, MICRO_STEPS_PER_1_STEP * 4);

cnt = 1;
for i in range (0, MICRO_STEPS_PER_1_STEP * 4):
	winding1.append (m.sin (f(i)));
	winding2.append (m.cos (f(i)));
	winding1O.append (m.sin (g[i]));
	winding2O.append (m.cos (g[i]));

factor = 4095 / max (winding1);

sys.stdout.write ("const int SINE[] = {\n\t");

for i in range (0, len (winding1)):
        sys.stdout.write ((str (int (round (factor * winding1[i]))) + ",").ljust (6));
	
	if cnt % 20 == 0:
		sys.stdout.write ('\n\t');

	cnt += 1;

# COSINE
sys.stdout.write ('\n};');
sys.stdout.write ('\n');
sys.stdout.write ('\n');
sys.stdout.write ('\n');
sys.stdout.write ("const int COSINE[] = {\n\t");

cnt = 1;
for i in range (0, len (winding2)):
        sys.stdout.write ((str (int (round (factor * winding2[i]))) + ",").ljust (6));
	
	if cnt % 20 == 0:
		sys.stdout.write ('\n\t');

	cnt += 1;

sys.stdout.write ('\n};');
sys.stdout.write ('\n');

plt.plot (range (0, MICRO_STEPS_PER_1_STEP * 4), winding1O, '-', 
	  range (0, MICRO_STEPS_PER_1_STEP * 4), winding2O, '-',  
	  range (0, MICRO_STEPS_PER_1_STEP * 4), winding1, '-',  
	  range (0, MICRO_STEPS_PER_1_STEP * 4), winding2, '-');

plt.legend (['sin', 'cos', 'sinMod', 'cosMod']);
plt.show ();


