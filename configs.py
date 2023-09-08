from enum import Enum
from typing import Dict

# ------------------ configs.py ------------------
# This file can be used to modify the benchmark parameters.
# Parameters are separated in two groups:
# - hash functions
# - collision strategy specific parameters
# Each benchmark will be executed
#     for all hash functions
#         for all collision strategies
#             for all overalloc values

class HashCategories(Enum):
    LEARNED = 0
    CLASSIC = 1
    PERFECT = 2

class CollisionCategories(Enum):
    LINEAR = 0
    CHAINED = 1
    CUCKOO = 2

class HashF(Enum):
    RMIHash = 0
    RadixSplineHash = 1
    PGMHash = 2
    MURMUR = 3
    MultPrime64 = 4
    FibonacciPrime64 = 5
    AquaHash = 6
    XXHash3 = 7
    MWHC = 8
    BitMWHC = 9
    RecSplit = 10

# Define available hash functions mapping
AVAILABLE_HASH_FUNCTIONS: Dict[HashF, HashCategories] = {
    # LEARNED
    HashF.RMIHash: HashCategories.LEARNED,
    HashF.RadixSplineHash: HashCategories.LEARNED,
    HashF.PGMHash: HashCategories.LEARNED,
    # CLASSIC
    HashF.MURMUR: HashCategories.CLASSIC,
    HashF.MultPrime64: HashCategories.CLASSIC,
    HashF.FibonacciPrime64: HashCategories.CLASSIC,
    HashF.AquaHash: HashCategories.CLASSIC,
    HashF.XXHash3: HashCategories.CLASSIC,
    # PERFECT
    HashF.MWHC: HashCategories.PERFECT,
    HashF.BitMWHC: HashCategories.PERFECT,
    HashF.RecSplit: HashCategories.PERFECT              # FIXME - not sure
}

# ========= START - modify these constants as you wish ========= //

# ------ HASH FUNCTIONS ------
# All the available hash functions are displayed in the AVAILABLE_HASH_FUNCTION dictionary.
# Each function is associated to its category (learned, classic or perfect). 
# TODO - uncomment
functions_config = [
    HashF.RMIHash, HashF.RadixSplineHash, # HashF.PGMHash,
    HashF.MURMUR, HashF.MultPrime64, HashF.FibonacciPrime64, HashF.AquaHash, HashF.XXHash3,
    HashF.MWHC, HashF.BitMWHC, # HashF.RecSplit
]

# ------ COLLISION MANAGEMENT ------
# BUCKET_SIZE
# The number of elements which are allowed in each bucket [default 1].
#     OVERALLOC
# TODO
#     KICKING_BIAS
# Controls the kicking strategy in the Cuckoo collision management strategy.
# If set to 0, then the BALANCED strategy is used. 
# Otherwise, the BIASED strategy is used, and KICKING_BIAS represents the value of the bias.
DEFAULT_BUCKET_SIZE = 1
# [ only for learned functions ]
DEFAULT_MAX_MODELS = 100
DEFAULT_MAX_ERRORS = 1024
num_radix_bits = 18

# LINEAR configuration
LINEAR_BUCKET_SIZE = DEFAULT_BUCKET_SIZE
LINEAR_OVERALLOC = [34, 54, 82, 122, 185, 300]
# [ only for learned functions ]
LINEAR_MAX_MODELS = DEFAULT_MAX_MODELS
LINEAR_MAX_ERRORS = DEFAULT_MAX_ERRORS

# CHAINED configuration
CHAINED_BUCKET_SIZE = DEFAULT_BUCKET_SIZE
CHAINED_OVERALLOC = [10050, 10067, 10080, 0, 34, 100, 300]
# [ only for learned functions ]
CHAINED_MAX_MODELS = DEFAULT_MAX_MODELS
CHAINED_MAX_ERRORS = DEFAULT_MAX_ERRORS

# CUCKOO configuration
CUCKOO_BUCKET_SIZE = 4
CUCKOO_OVERALLOC = [34, 25, 17, 11, 5]
KICKING_BIAS = 5
# [ only for learned functions ]
CUCKOO_MAX_MODELS = 100000
CUCKOO_MAX_ERRORS = 32

# Define configuration structs
class ConfigBM:
    def __init__(self, collision_strategy, bucket_size, overalloc, kicking_bias, max_models, max_errors):
        self.collision_strategy = collision_strategy
        self.bucket_size = bucket_size
        self.overalloc = overalloc
        self.kicking_bias = kicking_bias
        self.max_models = max_models
        self.max_errors = max_errors

# Configuration instances
linear_config = ConfigBM(CollisionCategories.LINEAR, LINEAR_BUCKET_SIZE, LINEAR_OVERALLOC, 0, LINEAR_MAX_MODELS, LINEAR_MAX_ERRORS)
chained_config = ConfigBM(CollisionCategories.CHAINED, CHAINED_BUCKET_SIZE, CHAINED_OVERALLOC, 0, CHAINED_MAX_MODELS, CHAINED_MAX_ERRORS)
cuckoo_config = ConfigBM(CollisionCategories.CUCKOO, CUCKOO_BUCKET_SIZE, CUCKOO_OVERALLOC, KICKING_BIAS, CUCKOO_MAX_MODELS, CUCKOO_MAX_ERRORS)
