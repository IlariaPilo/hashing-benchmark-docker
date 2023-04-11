import numpy as np
import struct
import os
import sys

def downsample(fn, directory, remove=False, start_M=800, end_M=200):
    start_path = directory + "/" + fn + "_" + str(start_M) + "M_uint64"
    end_path = directory + "/" + fn + "_" + str(end_M) + "M_uint64"

    if os.path.exists(end_path):
        print("Nothing to do here!\n")
        if remove:
            os.remove(start_path)
            print("Initial file removed\n")
        return
    if not os.path.exists(start_path):
        raise Exception("File does not exist! :(")
    
    step = start_M / end_M

    print("Downsampling", fn)
    print("Step:", step)

    print("\nstart reading file...")
    d = np.fromfile(start_path, dtype=np.uint64)[1:]
    print("...reading done!\n")

    nd = d[::int(step)]

    print("start writing file...")
    with open(end_path, "wb") as f:
        f.write(struct.pack("Q", len(nd)))
        nd.tofile(f)
    print("...writing done!\n")

    if remove:
        os.remove(start_path)
        print("Initial file removed\n")

# get the arguments
new_directory = sys.argv[1]
try:
    if sys.argv[2] == "remove":
        remove = True
    else:
        remove = False
except:
    remove = False

downsample("osm_cellids", new_directory, remove)