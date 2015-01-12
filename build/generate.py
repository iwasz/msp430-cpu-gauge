#!/usr/bin/env python

import numpy;
import math as m;
import sys

def mySin (x):
    return m.sin (x);
#    return m.sin (m.sin (x));
#    return m.sin (m.sin (m.sin (m.sin (m.sin (m.sin (x))))));
#    return (8 / m.pi ** 2) * (m.sin (x) - (1.0 / 9.0) * m.sin (3 * x));
#    return (8 / m.pi ** 2) * (m.sin (x) - (1.0 / 9.0) * m.sin (3 * x) + (1.0 / 25.0) * m.sin (5.0 * x));

def myCos (x):
#    return m.cos (x);
#    return m.sin (m.sin (m.sin (m.sin (m.sin (m.cos (x))))));
    return mySin (x + m.pi / 2.0);

winding1 = [];
winding2 = [];

cnt = 1;
#for i in numpy.arange (0, 2 * m.pi, m.pi / 256):
for i in numpy.arange (0, 2 * m.pi, m.pi / 64):
	winding1.append (mySin (i));
	winding2.append (myCos (i));

#factor = 4095 / max (winding1);
factor = 0xff / max (winding1);

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

