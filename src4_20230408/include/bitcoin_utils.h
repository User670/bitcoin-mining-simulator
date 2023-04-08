#ifndef BITCOIN_UTILS_H
#define BITCOIN_UTILS_H

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

/*typedef struct MerkleTreeNode{
    char hash[32];
    int is_data_node; // use as boolean
    struct MerkleTreeNode* left;
    struct MerkleTreeNode* right;
} MerkleTreeNode;*/

typedef struct MerkleTreeDataNode{
    int length;
    char* data;
}MerkleTreeDataNode;

// ###### DEPRECATED?
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

// ###### DEPRECATED
typedef struct BitcoinBlock{
    BitcoinHeader header;
    MerkleTreeHashNode* merkle_tree;
}BitcoinBlock;

// data points to an ARRAY of data nodes
typedef struct{
    BitcoinHeader header;
    int data_count;
    MerkleTreeDataNode* data;
}BitcoinBlockv2;

typedef struct BitcoinBlockv3{
    BitcoinHeader header;
    struct BitcoinBlockv3* previous_block;
    struct BitcoinBlockv3* next_block;
    MerkleTreeHashNode* merkle_tree;
}BitcoinBlockv3;

void dsha(void* message, unsigned int len, void* digest);

void construct_target(int d, void* target_storage);

int is_good_block(BitcoinHeader* block, const char* target);

void merkle_hash(void* a, void* b, void* digest);

void update_merkle_root(BitcoinBlock* block);
void update_merkle_root_v3(BitcoinBlockv3* block);

void calculate_merkle_root_top_down(MerkleTreeHashNode* node);

void update_merkle_root_v2(BitcoinBlockv2* block);

void calculate_merkle_root_v2(MerkleTreeDataNode* data, int count, char* target);

int count_transactions_v3(MerkleTreeHashNode* node);

int tree_is_full_v3(MerkleTreeHashNode* node);

int tree_depth_v3(MerkleTreeHashNode* node);

void initialize_hash_node(MerkleTreeHashNode* node);

void add_layer_v3(MerkleTreeHashNode* tree);

void add_data_node_v3(MerkleTreeHashNode* tree, MerkleTreeDataNode* node);

void construct_merkle_tree_v3(BitcoinBlockv3* block, int transaction_count, MerkleTreeDataNode* data);

void free_data_node(MerkleTreeDataNode* node);

void recursive_free_merkle_tree_v3(MerkleTreeHashNode* node);

void recursive_free_block_v3(BitcoinBlockv3* block);

void initialize_block_v3(BitcoinBlockv3* block, int difficulty);

void attach_block_v3(BitcoinBlockv3* genesis, BitcoinBlockv3* new_block);

#endif