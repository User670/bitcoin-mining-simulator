Convert a difficulty "bits" to approximately 1 in how many chance to guess the hash, and vice versa.

Benchmark reference: on my 2021 gaming laptop that costs 7000 yuan (~$1000), it takes **1.8 seconds to run 1 million hashes** (WSL Ubuntu on Windows 10). Virtual machine goes slightly slower at 1.9 seconds (Ubuntu on VMWare Workstation Player on Windows 10).

(Test program is single-process, single-threaded; loop only includes assigning nonce, calculating hash and comparing against a pre-calculated target, no other logic like checking signals included in the loop. CPU is `11th Gen Intel(R) Core(TM) i5-11400H @ 2.70GHz`)