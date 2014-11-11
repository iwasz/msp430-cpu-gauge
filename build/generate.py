#!/usr/bin/env python

import numpy;
import math;
import sys

def mySin (i):
    return math.sin (i);
#    return math.sin (math.sin (math.sin (math.sin (math.sin (math.sin (i))))));

def myCos (i):
    return math.cos (i);
#    return math.sin (math.sin (math.sin (math.sin (math.sin (math.cos (i))))));

winding1 = [];
winding2 = [];

cnt = 1;
for i in numpy.arange (0, 2 * math.pi, math.pi / 256):
	winding1.append (mySin (i));
	winding2.append (myCos (i));

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

