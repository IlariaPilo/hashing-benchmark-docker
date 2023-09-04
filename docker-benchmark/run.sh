#!/bin/bash

# Thanks @ ChatGPT for the skeleton of this bash program :D

fb="fb_200M_uint64"
osm="osm_cellids_200M_uint64"
wiki="wiki_ts_200M_uint64"

INITIAL_DIR=$(pwd)
_source_dir_=$(dirname "$0")
BASE_DIR=$(readlink -f "${_source_dir_}/..")     # /home/ilaria/Documents/stage/hashing-benchmark-docker

output_dir="${BASE_DIR}/output"

# Check if the user has provided an argument
if [ $# -eq 0 ]; then
  echo -e "\n\033[1;35m\tbash run.sh <input_dir> \033[0m"
  echo -e "Runs the previously built container."
  echo -e "<input_dir> contains the directory where the datasets are [or will be] saved.\n"
  exit
else
  input_dir=$(realpath $1)
fi

mkdir -p $input_dir
mkdir -p $output_dir

# Check if input_dir stores all datasets
if [[ $(ls $input_dir/{$fb,$osm,$wiki} 2>/dev/null | wc -l) -ne 3 ]]; then
    echo "It looks like the provided directory does not contain the required datasets."
    echo "Do you want to download them now? [y/N]"
    read answer

    if [[ $answer == "y" || $answer == "Y" ]]; then
        bash ${BASE_DIR}/scripts/setup_datasets.sh $input_dir
    else
        echo "Operation aborted"
        exit 1
    fi
fi

docker run --rm -v $output_dir:/home/benchmarker/hashing-benchmark-docker/output \
    -v $input_dir:/home/benchmarker/hashing-benchmark-docker/data \
    -it docker-benchmark