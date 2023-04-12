import sys
import os
import subprocess

conf = {
    "learned_linear" : {
        "bucket_size" : 1,
        "overalloc": [34, 54, 82, 122, 185, 300],
        "model_name" : ["RMIHash", "RadixSplineHash"],
        "params" : ["Model", "Linear", "Balanced", 0, 100, 1024]
    },
    "traditional_linear" : {
        "bucket_size" : 1,
        "overalloc": [34, 54, 82, 122, 185, 300],
        "model_name" : ["MURMUR", "MultPrime64"],
        "params" : ["Traditional", "Linear", "Balanced",0, 0, 0]
    },
    "perfect_linear" : {
        "bucket_size" : 1,
        "overalloc": [34, 54, 82, 122, 185, 300],
        "model_name" : ["MWHC", "BitMWHC"],
        "params" : ["Exotic", "Linear", "Balanced",0, 0, 0]
    },
    # ----------------------- #
    "learned_chained" : {
        "bucket_size" : 1,
        "overalloc": [10050, 10067, 10080, 0, 34, 100, 300],
        "model_name" : ["RMIHash", "RadixSplineHash"],
        "params" : ["Model", "Chained", "Balanced", 0, 100, 1024]
    },
    "traditional_chained" : {
        "bucket_size" : 1,
        "overalloc": [10050, 10067, 10080, 0, 34, 100, 300],
        "model_name" : ["MURMUR", "MultPrime64"],
        "params" : ["Traditional", "Chained", "Balanced", 0, 0, 0]
    },
    "perfect_chained" : {
        "bucket_size" : 1,
        "overalloc": [10050, 10067, 10080, 0, 34, 100, 300],
        "model_name" : ["MWHC", "BitMWHC"],
        "params" : ["Exotic", "Chained", "Balanced", 0, 0, 0]
    },
    # ----------------------- #
    "learned_cuckoo" : {
        "bucket_size" : 4,
        "overalloc": [34, 25, 17, 11, 5],
        "model_name" : ["RMIHash", "RadixSplineHash"],
        "params" : ["Model", "Cuckoo", "Biased", 5, 100000, 32]
    },
    "traditional_cuckoo" : {
        "bucket_size" : 4,
        "overalloc": [34, 25, 17, 11, 5],
        "model_name" : ["MURMUR", "MultPrime64"],
        "params" : ["Traditional", "Cuckoo", "Biased",5, 0, 0]
    },
    "perfect_cuckoo" : {
        "bucket_size" : 4,
        "overalloc": [34, 25, 17, 11, 5],
        "model_name" : ["MWHC", "BitMWHC"],
        "params" : ["Exotic", "Cuckoo", "Biased", 5, 0, 0]
    }
}

command = sys.argv[1]
struct = conf[command]
# Current directory is parallel/<command>
output_dir = os.path.abspath("../../output") + "/" + command


def tail(filename, remove=False):
    # Open file and read lines into a list
    with open(filename, "r") as file:
        lines = file.readlines()
    # Save last line
    last_line = lines[-1]
    # Check if the last line does not start with "Start Here"
    if not last_line.startswith("Start Here"):
        # Nothing to do here
        print(f"[{command}] Nothing to do here! *exits*")
        os._exit(0)
    if remove:
        # Remove last line from list
        lines.pop()

        # Write remaining lines back to file
        with open(filename, "w") as file:
            file.writelines(lines)
    return last_line

def load_checkpoint():
    # check if checkpoint file exists
    if not os.path.isfile(output_dir + "_results_tmp.json"):
        return 0, 0
    try:
        # get the last line
        last_line = tail(output_dir + "_results_tmp.json", remove=True)
        tokens = last_line.split()
        # tokens[0] = Start
        # tokens[1] = Here
        # tokens[2] = \bucket size\
        # tokens[3] = \overalloc\   <---
        # tokens[4] = \model name\  <---
        overalloc = struct["overalloc"].index(int(tokens[3]))
        model_name = struct["model_name"].index(tokens[4])
        # done!
        return overalloc, model_name
    except:
        return 0, 0

def run_script(start_overalloc = 0, start_model_name = 0):
    bucket_size = str(struct["bucket_size"])
    params_list = list(map(str, struct["params"]))
    params_str = ' '.join(map(str, struct["params"]))
    for io in range(start_overalloc, len(struct["overalloc"])):
        overalloc = str(struct["overalloc"][io])
        for im in range(start_model_name, len(struct["model_name"])):
            model_name = str(struct["model_name"][im])
            with open(output_dir + '_results_tmp.json', 'a') as f:
                f.write(f'Start Here {bucket_size} {overalloc} {model_name} {params_str}\n')
            with open(output_dir + '_log_stats.out', 'a') as f:
                f.write(f'Start Here {bucket_size} {overalloc} {model_name} {params_str}\n')
            subprocess.run(["python3", "edit_benchmark.py", bucket_size, overalloc, model_name, *params_list])
            subprocess.run(f'bash run.sh >> {output_dir}_log_stats.out', shell=True)
            subprocess.run(f'cat benchmark_results.json >> {output_dir}_results_tmp.json', shell=True)


print(f"[{command}] Loading checkpoint...")
# load the checkpoint...
start_overalloc, start_model_name = load_checkpoint()

if start_overalloc != 0 or start_model_name != 0:
    print(f"[{command}] Checkpoint loaded!")
else:
    print(f"[{command}] No checkpoint available!")

print(f"[{command}] Starting computation...")
# start!
run_script(start_overalloc, start_model_name)
print(f"[{command}] Done! *exits*")