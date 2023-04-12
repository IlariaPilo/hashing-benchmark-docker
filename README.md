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
```
cd docker-benchmark
bash build.sh
```
If everything goes according to plans, the image can be later run with `bash run.sh`. The script is intended to be used as follows:
```
Usage: run.sh [-i input_directory] [-h]
  -i input_directory: the directory that contains the datasets
     [default: /hashing-benchmark-docker/data]
  -h: Print this help message
```
Notice that **all directories refer to the host machine**.

The script automatically checks whether the input directory actually contains the dataset. If it does not, you can choose to download them or to abort the program.

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
cd hashing-benchmark-docker
bash setup_datasets.sh
```
The script also performs automatic downsampling and patches the `hashing-benchmark-docker/src/support/datasets.hpp`.

You can specify the directory where we want our data to be loaded:
```
bash setup_datasets.sh /home/ilaria/some_external_directory
```
If no directory is specified, a default `hashing-benchmark-docker/data` direcotry is created and used.

## Run the experiments
To run the hash table experiments, use the following commands:
```
cd code
python benchmark_parallel.py <number_of_threads>
```
If not specified, the number of threads is set to 9.

The results of the hash table experiments are stored in JSON format in `/output/YY-mm-dd-HH-MM_results.json`.

