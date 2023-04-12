import shutil
import os
import sys
import subprocess
from multiprocessing import Pool
from datetime import datetime

# Save the original working directory
original_dir = os.getcwd()

def create_copy_and_execute(element):

    # Create a fully recursive copy of a directory
    new_dir = f'../parallel/{element}_code'
    shutil.copytree('.', new_dir)
    
    # Move into the copied directory
    os.chdir(new_dir)
    
    # Call a Python script
    subprocess.call(['python3', 'benchmark_basic.py', element])

    os.chdir(original_dir)


if __name__ == '__main__':
    # List of elements to process in parallel
    targets = ["learned_linear", "traditional_linear", "perfect_linear", 
           "learned_chained","traditional_chained", "perfect_chained",
           "learned_cuckoo", "traditional_cuckoo", "perfect_cuckoo"]
    
    # Number of parallel processes to use
    if len(sys.argv) < 2 or int(sys.argv[1]) > 9:
        num_processes = 9
    else:
        num_processes = int(sys.argv[1])

    if not os.path.exists('../parallel'):
        # Create the directory
        os.mkdir('../parallel')
    
    # Create a process pool with the specified number of processes
    with Pool(num_processes) as pool:
        # Map the list of elements to the pool of processes
        pool.map(create_copy_and_execute, targets)

    # Cleanup
    # Get all output files
    # Define the list of file prefixes
    now = datetime.now()
    date_string = now.strftime("%Y-%m-%d-%H-%M")
    output_file = "../output/" + date_string + "_results.json"

    # Open the output file for writing
    with open(output_file, 'w') as outfile:

        # Iterate over the list of file prefixes
        for file_prefix in targets:
            f_name = file_prefix + "_results_tmp.json"

            # Open the input file for reading
            with open(f_name, 'r') as infile:
                # Read the contents of the input file and write it to the output file
                outfile.write(infile.read())
                # Add a newline character at the end of each file's content
                # outfile.write('\n')
                
            # Remove the temporary file
            os.remove(f_name)

