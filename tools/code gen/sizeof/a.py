types="""BitcoinBlockv3
MerkleTreeHashNode
MerkleTreeDataNode
BitcoinHeader""".split("\n")

for i in types:
    print(f'printf("{i}: %d\\n",sizeof({i}));')