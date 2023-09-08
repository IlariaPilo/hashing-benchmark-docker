# load_benchmarks.py
# Prepares the benchmark code when the config file is modified.

# 1 > copy code/src/benchmarks/template_tables.hpp into code/src/benchmarks/tables.hpp -> DONE
# 2 > add necessary lines
# 3 > add } // namespace _

import os
import sys

script_path = os.path.abspath(__file__)
script_directory = os.path.dirname(script_path)
BASE_DIR = os.path.abspath(f'{script_directory}/..')+'/'

# Import configuration file
sys.path.append(BASE_DIR)
from configs import *

# Import BM edit library
sys.path.append(BASE_DIR+'code/')
from edit_BM_lib import hash_line, kick_line, bench_line, function_names

# Copy the content of template_tables.hpp
with open(BASE_DIR+'code/src/benchmarks/template_tables.hpp', 'r') as source_file:
    source_content = source_file.read()

with open(BASE_DIR+'code/src/benchmarks/tables.hpp', 'w') as dest_file:
    dest_file.write(source_content)
    # Start generating things

    # kick line
    dest_file.write('\t// Kicking strategy (for Cuckoo)\n')
    kick_l = kick_line(cuckoo_config.kicking_bias)
    dest_file.write('\t'+kick_l+'\n\n')

    dest_file.write('\t// Function aliases definition\n')
    # hash line
    for f in functions_config:
        if AVAILABLE_HASH_FUNCTIONS[f] != HashCategories.LEARNED:
            hash_l = hash_line(f)
            dest_file.write('\t'+hash_l+'\n')
        else:
            hash_l = hash_line(f, chained_config, num_radix_bits)
            dest_file.write('\t'+hash_l+'\n')
            hash_l = hash_line(f, cuckoo_config, num_radix_bits)
            dest_file.write('\t'+hash_l+'\n')
            hash_l = hash_line(f, linear_config, num_radix_bits)
            dest_file.write('\t'+hash_l+'\n')

    dest_file.write('\n\t// Benchmarks definitions\n')

    # bench line
    for f in functions_config:
        dest_file.write(f'\n\t// --------------- {function_names[f]} --------------- // \n')
        dest_file.write('\t// Chained \n')
        bench_l = bench_line(f, chained_config)
        dest_file.write(bench_l+'\n')
        dest_file.write('\t// Cuckoo \n')
        bench_l = bench_line(f, cuckoo_config)
        dest_file.write(bench_l+'\n')
        dest_file.write('\t// Linear \n')
        bench_l = bench_line(f, linear_config)
        dest_file.write(bench_l+'\n')

    # add } // namespace _
    dest_file.write('}\t// namespace _')


print("DONE")
