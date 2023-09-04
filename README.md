# Hashing Benchmarking [DOCKER]

This fork adds the following features:
- Provide a Docker container to run the experiments in the proper environment;
- Include scripts to download and prepare the datasets automatically;
- Improve the benchmarking script to run experiments in parallel;
- Implement a checkpoint strategy.

__*[ Up to now, only hash table experiment support ]*__

To use this version of the repository, just follow these steps.

## Clone the repository
The repository can be cloned by running `git clone --recurse-submodules https://github.com/IlariaPilo/hashing-benchmark-docker`.

## Use the Docker Image
Build the Docker Image with:
```bash
cd docker-benchmark
bash build.sh
```
If everything goes according to plans, the image can be later run with `bash run.sh`. The script is intended to be used as follows:
```bash
bash run.sh <input_dir>
```
where `<input_dir>` refers to the directory storing the required datasets. 

The script automatically checks whether the input directory actually contains the dataset. If it does not, you can choose to download them or to abort the program.

Notice that **all directories refer to the host machine**.

The generated credentials are (with `sudo` permissions):
```
USER: benchmarker
PASSWORD: password
```

__*IMPORTANT:*__ never, ever, _ever_ update the package manager on the Docker container!

## Download the datasets
If you don't want to use the container, and you prefer running the experiments on your native environment, you can use this utility to download and setup the datasets.

In order to download the datasets used by the benchmark script, simply run:
```
cd scripts
bash setup_datasets.sh
```
The script also performs automatic downsampling and patches the `/code/src/support/datasets.hpp`.

You can specify the directory where we want our data to be loaded:
```
bash setup_datasets.sh /home/ilaria/some_external_directory
```
If no directory is specified, a default `/data` direcotry is created and used.

## Run the experiments
To run the hash table experiments, use the following commands:
```
cd scripts
bash benchmark.sh <number_of_threads>
```
If not specified, the number of threads is set to 9. The output of each job is saved in a separate `/output/*results_tmp.json` file.

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
