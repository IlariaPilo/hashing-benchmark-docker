import shutil
import os
import sys
import subprocess
from multiprocessing import Pool

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
