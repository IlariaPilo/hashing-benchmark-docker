import sys

# TODO - get rid of this horrible thing
hash_mapping_dict={
    "MURMUR":"using MURMUR = hashing::MurmurFinalizer<Key>;",
    "MultPrime64":"using MultPrime64 = hashing::MultPrime64;",
    "FibonacciPrime64":"using FibonacciPrime64 = hashing::FibonacciPrime64;",
    "AquaHash":"using AquaHash = hashing::AquaHash<Key>;",
    "XXHash3":"using XXHash3 = hashing::XXHash3<Key>;",
    "MWHC":"using MWHC = exotic_hashing::MWHC<Key>;",
    "BitMWHC":"using BitMWHC = exotic_hashing::BitMWHC<Key>;",
    "FST":"using FST = exotic_hashing::FastSuccinctTrie<Data>;",
    "RadixSplineHash":"using RadixSplineHash = learned_hashing::RadixSplineHash<std::uint64_t,num_radix_bits,max_error,100000000>;",
    "RMIHash":"using RMIHash = learned_hashing::RMIHash<std::uint64_t,max_models>;"
}
model_type_dict={
    "Traditional":"",
    "Exotic":"Exotic",
    "Model":"Model"

}
scheme_dict={
    "Cuckoo":"Cuckoo",
    "Linear":"Linear",
    "Chained":"Chained"
}
kickin_strat_dict={
    "Balanced":"using KickingStrat = kapilmodelhashtable::KapilModelBalancedKicking;",
    "Biased":"using KickingStrat = kapilmodelhashtable::KapilModelBiasedKicking<kickinit_strat_bias>;"
}

print(f"\033[31msys.argv: \033[0m{sys.argv}")

# number of elements which are allowed in every bucket (1 for everything but cuckoo hashing - 4)
bucket_size=int(sys.argv[1]) 
# TODO - boh? -------------------
overalloc=int(sys.argv[2])
# the name of the model we are using (eg, RMIHash, RadixSplineHash)
model_name=str(sys.argv[3])
# the type of the model we are using, which can be either
# - "Model": a learned function based model (RMIHash, RadixSplineHash)
# - "Traditional": a classic hash function (MURMUR hashing, MultiplyPrime...)
# - "Exotic": a perfect hash function (MWHC)
model_type=str(sys.argv[4])
# collision management strategy (Chained, Linear, Cuckoo)
hashing_scheme=str(sys.argv[5])
# kicking strategy (Balanced, Biased) - only for cuckoo? TODO
kickinit_strat=str(sys.argv[6])
# bias value (??) TODO
kickinit_strat_bias=int(sys.argv[7])
# max number of models (only for learned)
max_models=int(sys.argv[8])
# max error (only for learned)
max_error=int(sys.argv[9])
num_radix_bits=18


def hash_line(model_name,model_type,max_models,max_error,num_radix_bits):
    if model_type_dict[model_type]!="Model":
        return hash_mapping_dict[model_name]
    ans_str=hash_mapping_dict[model_name]
    if "max_models" in ans_str:
        ans_str=ans_str.replace("max_models",str(max_models))
    if "max_error" in ans_str:
        ans_str=ans_str.replace("max_error",str(max_error)) 
    if "num_radix_bits" in ans_str:
        ans_str=ans_str.replace("num_radix_bits",str(num_radix_bits))        
    return ans_str 
# examples
# hash_line(MWHC,Exotic,0,0,18)        -> using MWHC = exotic_hashing::MWHC<Key>;
# hash_line(RMIHash,Model,100,1024,18) -> using RMIHash = learned_hashing::RMIHash<std::uint64_t,100>;


def kick_line(hashing_scheme,kickinit_strat,kickinit_strat_bias):
    if scheme_dict[hashing_scheme]!="Cuckoo":
        return ""
    ans_str=kickin_strat_dict[kickinit_strat]
    if  "kickinit_strat_bias" in ans_str:
        ans_str=ans_str.replace("kickinit_strat_bias",str(kickinit_strat_bias))
    return ans_str
# examples
# kick_line(Cuckoo,Biased,5)    -> using KickingStrat = kapilmodelhashtable::KapilModelBiasedKicking<5>;
# kick_line(Chained,Balanced,0) -> <empty string>    


def bench_line(hashing_scheme,model_type,bucket_size,overalloc):
    ans_str="BenchmarKapil"+scheme_dict[hashing_scheme]+model_type_dict[model_type]
    ans_str+="("+str(bucket_size)+","+str(overalloc)+","+model_name
    if scheme_dict[hashing_scheme]=="Cuckoo":
        ans_str+=",KickingStrat);"
    else:
        ans_str+=");"
    return ans_str   
# examples
# bench_line(Cuckoo,Exotic,4,34)  -> BenchmarKapilCuckooExotic(4,34,MWHC,KickingStrat);
# bench_line(Chained,Exotic,4,34) -> BenchmarKapilChainedExotic(4,34,MWHC,KickingStrat);


# ----------------------------- MAIN ----------------------------- # 

file1=open("src/benchmarks/template_tables.hpp","r")
file2=open("src/benchmarks/tables.hpp","w")

# copy the template_tables into the tables one
for line in file1.readlines():
    file2.write(line)

# Release used resources
file1.close()
file2.close()

# modify all things according to the functions
with open("src/benchmarks/tables.hpp", "a") as myfile:
    write_str=hash_line(model_name,model_type,max_models,max_error,num_radix_bits)+"\n"
    write_str+=kick_line(hashing_scheme,kickinit_strat,kickinit_strat_bias)+"\n"
    write_str+=bench_line(hashing_scheme,model_type,bucket_size,overalloc)
    write_str+="\n\n\n\n}  // namespace _\n"
    myfile.write(write_str)
