#!/bin/bash

#============ from https://github.com/learnedsystems/SOSD/blob/master/scripts/download.sh ============#

# Calculate md5 checksum of FILE and stores it in MD5_RESULT
function get_checksum() {
   FILE=$1

   if [ -x "$(command -v md5sum)" ]; then
      # Linux
      MD5_RESULT=`md5sum ${FILE} | awk '{ print $1 }'`
   else
      # OS X
      MD5_RESULT=`md5 -q ${FILE}`
   fi
}

function download_file_zst() {
   FILE=$1;
   CHECKSUM=$2;
   URL=$3;

   # Check if file already exists
   if [ -f ${FILE} ]; then
      # Exists -> check the checksum
      get_checksum ${FILE}
      if [ "${MD5_RESULT}" != "${CHECKSUM}" ]; then
         wget -O - ${URL} | zstd -d > ${FILE}
      fi
   else
      # Does not exists -> download
      wget -O - ${URL} | zstd -d > ${FILE}
   fi

   # Validate (at this point the file should really exist)
   get_checksum ${FILE}
   if [ "${MD5_RESULT}" != "${CHECKSUM}" ]; then
      echo "error checksum does not match: run download again"
      exit -1
   else
      echo ${FILE} "checksum ok"
   fi
}

#=====================================================================================================#


# Check if the user has provided an argument
if [ $# -eq 0 ]; then
    echo "Using default data directory: ./data"
    new_directory="./data"
else
    new_directory=$1
fi

# Save current directory
initial_dir=$(pwd)

# A file to setup the datasets
# To be used only once

# First, download the datasets

#============ from https://github.com/learnedsystems/SOSD/blob/master/scripts/download.sh ============#

echo "downloading data ..."
mkdir -p $new_directory
cd $new_directory

# Format: download_file <file_name> <md5_checksum> <url>
download_file_zst wiki_ts_200M_uint64 4f1402b1c476d67f77d2da4955432f7d https://dataverse.harvard.edu/api/access/datafile/:persistentId?persistentId=doi:10.7910/DVN/JGVF9A/SVN8PI
# download_file_zst books_200M_uint32 9f3e578671e5c0348cdddc9c68946770 https://dataverse.harvard.edu/api/access/datafile/:persistentId?persistentId=doi:10.7910/DVN/JGVF9A/5YTV8K  
download_file_zst osm_cellids_800M_uint64 70670bf41196b9591e07d0128a281b9a https://www.dropbox.com/s/j1d4ufn4fyb4po2/osm_cellids_800M_uint64.zst?dl=1
download_file_zst fb_200M_uint64 3b0f820caa0d62150e87ce94ec989978 https://dataverse.harvard.edu/api/access/datafile/:persistentId?persistentId=doi:10.7910/DVN/JGVF9A/EATHF7  

cd $initial_dir
echo "done"

#=====================================================================================================#

# Now, downsample osm_cellids to 200M
python downsample.py $new_directory remove

# Patch the /src/support/datasets.hpp file (-i = in place)
sed -i "s,/home/kapil/PhDAcads/benchmark_hashing/SOSD/scripts/data,${new_directory},g" ./src/support/datasets.hpp

