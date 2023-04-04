# docker-benchmark module

To use this version of the repository, just follow these steps.

## 0. Clone the repository
The repository can be cloned by running `git clone --recurse-submodules https://github.com/IlariaPilo/hashing-benchmark-docker`.

## 1. Build and run the Docker Image
After the repository has been cloned, build the Docker Image with:
```
cd docker-benchmark
bash build.sh
```
If everything goes according to plans, the image can be later run with `bash run.sh`. Now we are in the container!

The generated credentials are (with `sudo` permissions):
```
USER: benchmarker
PASSWORD: password
```

__*IMPORTANT:*__ never, ever, _ever_ update the package manager on the Docker!

## 2. Download the datasets
In order to download the datasets used by the benchmark script, simply run `bash setup_datasets.sh`. The script also performs automatic downsampling and patches the `hashing-benchmark-docker/src/support/datasets.hpp`.

We can specify the directory where we want our data to be loaded:
```
bash setup_datasets.sh /home/ilaria/some_external_directory
```
If no directory is specified, a default `hashing-benchmark-docker/data` direcotry is created and used.

## 3. We are done!
Hopefully, now experiments can be run as described in the [original README](../README.md).

