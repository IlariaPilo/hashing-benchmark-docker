import os
from datetime import datetime
# Thanks ChatGPT for this nice script!

# List of file prefixes to process
#targets = ["learned_linear", "traditional_linear", "perfect_linear", 
#        "learned_chained","traditional_chained", "perfect_chained",
#        "learned_cuckoo", "traditional_cuckoo", "perfect_cuckoo"]
targets = ["learned_linear", "traditional_linear", "perfect_linear", 
        "traditional_chained", "perfect_chained",
        "learned_cuckoo", "traditional_cuckoo"]
now = datetime.now()
date_string = now.strftime("%Y-%m-%d-%H-%M")
output_file = "../output/" + date_string + "_results.json"

with open(output_file, "w") as f_out:
    # Loop over each file prefix
    for prefix in targets:
        # Input and output filenames
        input_file = "../output/" + prefix + "_results_tmp.json"

        # Open input and output files
        with open(input_file, "r") as f_in:
            # Loop over each line in the input file
            for line in f_in:
                # If the line doesn't start with "Start Here", copy it to the output file
                if not line.startswith("Start Here"):
                    f_out.write(line)
