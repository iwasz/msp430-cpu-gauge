#!/usr/bin/env python

import numpy as np;
import math as m;
import sys
from scipy.interpolate import interp1d
import matplotlib.pyplot as plt;
import csv;

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

#print (y);
#sys.exit (0);

f = interp1d(x, y);
x2 = np.linspace(0, x[-1], 512);

#plt.plot (x, y, 'o', x2, f(x2), '-');
#plt.legend (['original', 'interpolated']);
#plt.show ();

# --------------------

winding1 = [];
winding2 = [];

winding1O = [];
winding2O = [];

g = np.linspace(0, 2*m.pi, 512);

cnt = 1;
for i in range (0, 512):
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

plt.plot (range (0, 512), winding1O, '-', 
	  range (0, 512), winding2O, '-',  
	  range (0, 512), winding1, '-',  
	  range (0, 512), winding2, '-');

plt.legend (['sin', 'cos', 'sinMod', 'cosMod']);
plt.show ();


