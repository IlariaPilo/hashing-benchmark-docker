import sys
import os

script_path = os.path.abspath(__file__)
script_directory = os.path.dirname(script_path)
BASE_DIR = os.path.abspath(f'{script_directory}/..')+'/'

# Import configuration file
sys.path.append(BASE_DIR)
from configs import HashCategories, HashF, CollisionCategories, AVAILABLE_HASH_FUNCTIONS, ConfigBM

function_names = {
    HashF.MURMUR: "MURMUR",
    HashF.MultPrime64: "MultPrime64",
    HashF.FibonacciPrime64: "FibonacciPrime64",
    HashF.AquaHash: "AquaHash",
    HashF.XXHash3: "XXHash3",
    HashF.MWHC: "MWHC",
    HashF.BitMWHC: "BitMWHC",
    HashF.RecSplit: "RecSplit",     # TODO - maybe
    # "FST":,
    HashF.RadixSplineHash: "RadixSplineHash",
    HashF.RMIHash: "RMIHash",
    HashF.PGMHash: "PGMHash"
}
hash_mapping_dict={
    HashF.MURMUR:"using MURMUR = hashing::MurmurFinalizer<Key>;",
    HashF.MultPrime64:"using MultPrime64 = hashing::MultPrime64;",
    HashF.FibonacciPrime64:"using FibonacciPrime64 = hashing::FibonacciPrime64;",
    HashF.AquaHash:"using AquaHash = hashing::AquaHash<Key>;",
    HashF.XXHash3:"using XXHash3 = hashing::XXHash3<Key>;",
    HashF.MWHC:"using MWHC = exotic_hashing::MWHC<Key>;",
    HashF.BitMWHC:"using BitMWHC = exotic_hashing::BitMWHC<Key>;",
    HashF.RecSplit:"",          # TODO - maybe
    # "FST":"using FST = exotic_hashing::FastSuccinctTrie<Data>;",
    HashF.RadixSplineHash:"using RadixSplineHash__count__ = learned_hashing::RadixSplineHash<std::uint64_t,num_radix_bits,max_error,100000000>;",
    HashF.RMIHash:"using RMIHash__count__ = learned_hashing::RMIHash<std::uint64_t,max_models>;",
    HashF.PGMHash:""            # TODO - add
}
function_type_dict={
    HashCategories.CLASSIC:"",
    HashCategories.PERFECT:"Exotic",
    HashCategories.LEARNED:"Model"
}
scheme_dict={
    CollisionCategories.CUCKOO:"Cuckoo",
    CollisionCategories.LINEAR:"Linear",
    CollisionCategories.CHAINED:"Chained"
}

def hash_line(f: HashF,collision_config: ConfigBM = None, num_radix_bits=None):
    f_type = AVAILABLE_HASH_FUNCTIONS[f]
    if f_type != HashCategories.LEARNED:
        return hash_mapping_dict[f]
    
    collision_strategy = collision_config.collision_strategy
    max_models = collision_config.max_models
    max_error = collision_config.max_errors
    ans_str=hash_mapping_dict[f]
    if "max_models" in ans_str:
        ans_str=ans_str.replace("max_models",str(max_models))
    if "max_error" in ans_str:
        ans_str=ans_str.replace("max_error",str(max_error)) 
    if "num_radix_bits" in ans_str:
        ans_str=ans_str.replace("num_radix_bits",str(num_radix_bits))        
    return ans_str.replace("__count__",scheme_dict[collision_strategy])
# examples
# hash_line(MWHC,Exotic,0,0,18)        -> using MWHC = exotic_hashing::MWHC<Key>;
# hash_line(RMIHash,Model,100,1024,18) -> using RMIHash = learned_hashing::RMIHash<std::uint64_t,100>;

def kick_line(kickinit_strat_bias):
    if kickinit_strat_bias == 0:
        return "using KickingStrat = kapilmodelhashtable::KapilModelBalancedKicking;"
    else:
        return f'using KickingStrat = kapilmodelhashtable::KapilModelBiasedKicking<{kickinit_strat_bias}>;'
# examples
# kick_line(Cuckoo,Biased,5)    -> using KickingStrat = kapilmodelhashtable::KapilModelBiasedKicking<5>;
# kick_line(Chained,Balanced,0) -> <empty string>    

def bench_line(f: HashF,collision_config: ConfigBM):
    f_name = function_names[f]
    f_type = AVAILABLE_HASH_FUNCTIONS[f]
    f_type_str = function_type_dict[f_type]
    collision_strategy = scheme_dict[collision_config.collision_strategy]
    bucket_size = collision_config.bucket_size
    ans_str = ''
    for overalloc in collision_config.overalloc:
        ans_str+="\tBenchmarKapil"+collision_strategy+f_type_str+"("+str(bucket_size)+","+str(overalloc)+","+f_name
        if f_type == HashCategories.LEARNED:
            ans_str+=collision_strategy
        if collision_strategy == CollisionCategories.CUCKOO:
            ans_str+=",KickingStrat);\n"
        else:
            ans_str+=");\n"
    return ans_str   
# examples
# bench_line(Cuckoo,Exotic,4,34)  -> BenchmarKapilCuckooExotic(4,34,MWHC,KickingStrat);
# bench_line(Chained,Exotic,4,34) -> BenchmarKapilChainedExotic(4,34,MWHC);
