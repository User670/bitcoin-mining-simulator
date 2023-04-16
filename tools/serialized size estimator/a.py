# This thing is outdated!
# It was written for BitcoinBlockv3, which has to be serialized and deserialized using a convoluted process.
# Current BitcoinBlockv4 takes a fixed amount of space (around 15KB; has space for 30 transactions,
# each being up to 512 bytes).

chain_length=int(input("max length of blockchain? > "))
tree_size=int(input("max # of transactions per block? > "))
transaction_length=int(input("max transaction length? > "))

def hash_node_count(n):
    r=n+1
    while n>1:
        n=n//2+n%2
        r+=n
    return r

def size_display(b):
    names="bytes KB MB GB TB".split(" ")
    i=0
    while b>=1024:
        b/=1024
        i+=1
    return "{:.2f} {}".format(b, names[i])

s=4+chain_length*(80+4+tree_size*(4+transaction_length))

print("You need to allocate a buffer of at least {} bytes".format(s))
if s>1024:
    print("(which is {})".format(size_display(s)))

# block is 104 bytes, 80 of which is header.
# hash node is 56 bytes.
# data node is 16.
s=chain_length*(104+56*hash_node_count(tree_size)+tree_size*(16+transaction_length))
print("The chain will take up up to {} bytes in memory".format(s))
if s>1024:
    print("(which is {})".format(size_display(s)))
