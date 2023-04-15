import json
import sys
import os

if len(sys.argv) < 2:
    print("Usage: python clean_output.py <result_file_name>")

in_file_path = sys.argv[1]
out_file_path = os.path.splitext(in_file_path)[0]+ "_cleaned.json"

with open(in_file_path, 'r', encoding='utf-8') as in_file:
    # load json
    raw_json = json.load(in_file)

    # filter out duplicates, keeping best datapoint
    methods = {}
    for new_dp in raw_json["benchmarks"]:
        name = new_dp["name"]

        if name not in methods or float(methods[name]["cpu_time"]) > float(new_dp["cpu_time"]):
            methods[name] = new_dp

    # write out results
    raw_json["benchmarks"] = [methods[m] for m in sorted(methods)]
    with open(out_file_path, 'w', encoding='utf-8') as out_file:
        json.dump(raw_json, out_file, indent=2)
