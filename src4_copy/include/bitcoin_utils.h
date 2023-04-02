#ifndef BITCOIN_UTILS_H
#define BITCOIN_UTILS_H
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

// ###### DEPRECATED
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
    struct BitcoinBlockv3* prev_block;
    struct BitcoinBlockv3* next_block;
    MerkleTreeHashNode* merkle_tree;
}BitcoinBlockv3;

void dsha(void* message, unsigned int len, void* digest);

void construct_target(int d, void* target_storage);

int is_good_block(BitcoinHeader* block, const char* target);

void merkle_hash(void* a, void* b, void* digest);

void update_merkle_root(BitcoinBlock* block);

void calculate_merkle_root_top_down(MerkleTreeHashNode* node);

void update_merkle_root_v2(BitcoinBlockv2* block);

void calculate_merkle_root_v2(MerkleTreeDataNode* data, int count, char* target);

#endif