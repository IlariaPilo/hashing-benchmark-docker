# docker-benchmark module

To use this version of the repository, just follow these steps.

[__*Coming soon:*__ join experiment support]

## Clone the repository
The repository can be cloned by running `git clone --recurse-submodules https://github.com/IlariaPilo/hashing-benchmark-docker`.

## Use the Docker Image
Build the Docker Image with:
```
cd docker-benchmark
bash build.sh
```
If everything goes according to plans, the image can be later run with `bash run.sh`. Hopefully, now experiments can be reproduced as described in the [original README](../README.md).

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

