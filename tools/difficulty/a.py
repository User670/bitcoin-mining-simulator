hash_possibilities=256**32

def difficulty_to_target(d):
    mantissa=d%0x1000000
    exponent=d//0x1000000
    return mantissa*(256**(exponent-3))

def target_to_difficulty(t):
    exponent=3
    while t>0x7fffff:
        t//=256
        exponent+=1
    return exponent*0x1000000+t

opcode=input("Convert difficulty to chance (1), or convert chance to difficulty (2)? > ")
if opcode=="1":
    d=int(input("Difficulty in hexadecimal, without the 0x prefix > "),16)
    target=difficulty_to_target(d)
    chance=hash_possibilities//target
    print(f"It's about 1 in {chance} chance to guess a valid hash in any given try.")
    if chance>=2147482648:
        print(f"(This overflows int32 {chance//2147483648} times!)")
    elif chance>=1000000000:
        print("(You are risking overflowing int32!)")
elif opcode=="2":
    print("Benchmark reference: on my 2021 gaming laptop that costs 7000 yuan, it took a single thread 1.8 seconds to run 1 million hashes.")
    c=int(float(input("1 in how many chance to guess a valid hash? > ")))
    t=hash_possibilities//c
    d=target_to_difficulty(t)
    print("Approximate difficulty is", hex(d))
else:
    print("Invalid opcode")