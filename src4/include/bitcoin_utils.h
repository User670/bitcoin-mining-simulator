#ifndef BITCOIN_UTILS_H
#define BITCOIN_UTILS_H

#define DIFFICULTY_5M 0x1e035afe
#define DIFFICULTY_1M 0x1e10c6f7
#define DIFFICULTY_500K 0x1e218def
#define DIFFICULTY_100K 0x1f00a7c5
#define DIFFICULTY_30K 0x1f022f3d

typedef struct BitcoinHeader{
    int version;
    char previous_block_hash[32];
    char merkle_root[32];
    int timestamp;
    int difficulty;
    int nonce;
} BitcoinHeader;

typedef struct MerkleTreeDataNode{
    int length;
    char* data;
}MerkleTreeDataNode;

// `data` not NULL -> this node is a bottom layer node, `hash` holds hash of
// the data stored in the data node
// `data` NULL -> this node is not a bottom layer node, `hash` holds merkle
// hash of `left` and `right`
// if a not-bottom layer node only has one child, put it on `left`, and leave
// `right` NULL
typedef struct MerkleTreeHashNode{
    char hash[32];
    struct MerkleTreeHashNode* left;
    struct MerkleTreeHashNode* right;
    struct MerkleTreeDataNode* data;
} MerkleTreeHashNode;

typedef struct BitcoinBlock{
    BitcoinHeader header;
    struct BitcoinBlock* previous_block;
    struct BitcoinBlock* next_block;
    MerkleTreeHashNode* merkle_tree;
}BitcoinBlock;

void dsha(void* message, unsigned int len, void* digest);

void construct_target(int d, void* target_storage);

int is_good_block(BitcoinHeader* header, const char* target);

void merkle_hash(void* a, void* b, void* digest);

void update_merkle_root(BitcoinBlock* block);

void calculate_merkle_root_top_down(MerkleTreeHashNode* node);

int count_transactions(MerkleTreeHashNode* node);

int tree_is_full(MerkleTreeHashNode* node);

int tree_depth(MerkleTreeHashNode* node);

void initialize_hash_node(MerkleTreeHashNode* node);

void add_layer(MerkleTreeHashNode* tree);

void add_data_node(MerkleTreeHashNode* tree, MerkleTreeDataNode* node);

void construct_merkle_tree(BitcoinBlock* block, int transaction_count, MerkleTreeDataNode* data);

void free_data_node(MerkleTreeDataNode* node);

void recursive_free_merkle_tree(MerkleTreeHashNode* node);

void recursive_free_block(BitcoinBlock* block);

void recursive_free_blockchain(BitcoinBlock* block);

void initialize_block(BitcoinBlock* block, int difficulty);

void attach_block(BitcoinBlock* genesis, BitcoinBlock* new_block);

#endif