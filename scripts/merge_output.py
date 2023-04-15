from datetime import datetime
import sys
import os
import glob
import json
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

# 1.
# First, we clean and concatenate all tmp files
json_objects = []
# Loop over each file prefix
for prefix in targets:
    input_file = "../output/" + prefix + "_results_tmp.json"

    # Open input and output files
    with open(input_file, "r") as f_in:
        lines = f_in.readlines()
        # Replace "}Start Here" with "}\n]\n}\n"
        # Also check last line
        current_string = ''
        for i in range(len(lines)):
            if lines[i].strip().startswith("}Start Here"):
                lines[i] = "}\n]\n}\n"
            if lines[i].strip().startswith("Start Here"):
                lines[i] = ""
            current_string += lines[i]
            if lines[i] == '}\n' or lines[i] == "}\n]\n}\n":
                json_objects.append(current_string)
                current_string = ''
        # Check last line
        if lines[-1] != '}\n' or lines[-2].lstrip() != ']\n' or lines[-3].lstrip() != '}\n':
            current_string += "]\n}\n"

# 2. 
# Cleanup!
if len(sys.argv) > 1 and sys.argv[1] == "-rm":
    # Remove all files with the pattern '*results_tmp.json'
    for file_path in glob.glob('../output/*results_tmp.json'):
        os.remove(file_path)

    # Remove all files with the pattern '*.out'
    for file_path in glob.glob('../output/*.out'):
        os.remove(file_path)


# 3.
# Join everything
benchmarks = []
for json_object in json_objects:
    # Parse the JSON object and add its benchmarks to the list
    data = json.loads(json_object)
    benchmarks.extend(data["benchmarks"])

# Create a dictionary with the merged data
merged_data = {
    "context": data["context"],  # Use the context from the last file processed
    "benchmarks": benchmarks
}

# Save the merged data to a JSON file
with open(output_file, "w") as f:
    json.dump(merged_data, f, indent=2)
