import tempfile, os
from itertools import groupby, count

import numpy as np
data = np.genfromtxt("yourfile.dat",delimiter="\n")

temp_dir = tempfile.mkdtemp()

def tempfile_split(filename, temp_dir, chunk=5000000):
    with open(filename, 'r') as datafile:
        groups = groupby(datafile, key=lambda k, line=count(): next(line) // chunk)
        for k, group in groups:
            output_name = os.path.normpath(os.path.join(temp_dir + os.sep, "all_%s.dta" % k))
            with open(output_name, 'a') as outfile:
                outfile.write(''.join(group))

tempfile_split('C:\\Users\\StephanZheng\\CS156b\\um\\um\\all.dta', 'C:\\Users\\StephanZheng\\CS156b\\um\\um\\temp')