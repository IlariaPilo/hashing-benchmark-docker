# Hashing Benchmarking [DOCKER]

This fork adds the following features:
- Provide a Docker container to run the experiments in the proper environment;
- Include scripts to download and prepare the datasets automatically;
- Improve the benchmarking script to run experiments in parallel;
- Implement a checkpoint strategy.

__*[ Up to now, only hash table experiment support ]*__

<!--## Table Of Contents-->

To use this version of the repository, just follow these steps.

## 0 | Clone the repository
The repository can be cloned by running `git clone --recurse-submodules https://github.com/IlariaPilo/hashing-benchmark-docker`.

## 1 | Use the Docker Image
Build the Docker Image with:
```bash
cd docker
bash build.sh
```
If everything goes according to plans, the image can be later run with `bash run.sh`. The script is intended to be used as follows:
```bash
bash run.sh <input_dir> [--fast]
```
where `<input_dir>` refers to the directory that stores (or will store) the required datasets. 
The `--fast` [or `-f`] option skips the checksum control for a faster (but less safe) run of the container.

The script automatically checks whether the input directory actually contains the dataset. If it does not, it is possible to download them or to abort the program.

Notice that **all directories refer to the host machine**. <!-- TODO : maybe remove this part? -->

The generated credentials are (with `sudo` permissions):
```
USER: benchmarker
PASSWORD: password
```

__*IMPORTANT:*__ never, ever, _ever_ update the package manager on the Docker container!

<!-- everything is fine -->

## Download the datasets
<!-- TODO : maybe remove this part? -->
If you don't want to use the container, and you prefer running the experiments on your native environment, you can use this utility to download and setup the datasets.

In order to download the datasets used by the benchmark script, simply run:
```sh
cd scripts
bash setup_datasets.sh <input_dir>
```
where `<input_dir>` refers to the directory that will store the required datasets. 

The script also patches the `/code/src/support/datasets.hpp`.

## Run the experiments
To run the hash table experiments, use the following commands:
```
cd scripts
bash benchmark.sh <number_of_threads>
```
If not specified, the number of threads is set to 9. The output of each job is saved in a separate `/output/*results_tmp.json` file.

### Enable prints
Informative prints are disabled by default, as they make the output messy and less clear. To rehabilitate them, just change the value of the `PRINT` macro in the [./code/masters_thesis.hpp](masters_thesis.hpp) file to 1.
```diff
- #define PRINT 0
+ #define PRINT 1 
```

## Clean the output
It is possible to clean the output using the following scripts:
### 1. `merge_ouptut.py`
`merge_ouptut.py` combines all benchmarks in all `*results_tmp.json` files in a single JSON object. The object is dumped in `/output/YY-mm-dd-HH-MM_results.json`.

The option `-rm` can be specified to delete all `*results_tmp.json` and `.out` files.
```
cd scripts
python merge_output.py [-rm]
```
### 2. `clean_ouptut.py`
`clean_ouptut.py` compares different runs of the same benchmark, keeping only the best one. The cleaned results are saved in `/output/YY-mm-dd-HH-MM_results_cleaned.json`, where the date is the same of the input file.

Example of usage:
```
cd scripts
python clean_output.py ../output/2023-04-17-17-21_results.json
```

### 3. `export_ouptut.py`
`export_ouptut.py` plots many interesting charts. The output is an html file with the same name of the input file, saved in `/output/docs`.

Example of usage:
```
cd scripts
python export_output.py ../output/2023-04-17-17-21_results_cleaned.json
```
