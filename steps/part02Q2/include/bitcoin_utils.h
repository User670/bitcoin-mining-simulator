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

// ######DEPRECATED
typedef struct MerkleTreeDataNode{
    int length;
    char* data;
}MerkleTreeDataNode;

// ######DEPRECATED
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

/* // ######DEPRECATED
typedef struct BitcoinBlock{
    BitcoinHeader header;
    struct BitcoinBlock* previous_block;
    struct BitcoinBlock* next_block;
    MerkleTreeHashNode* merkle_tree;
}BitcoinBlock; */

typedef struct{
    int length;
    char data[512];
} MerkleTreeNode; // not to be confused with the other two

typedef struct{
    BitcoinHeader header;
    // name of the shared memory that holds the previous/next block
    // ideally should make them null-terminated strings...
    char previous_block[100];
    char next_block[100];
    int tree_length;
    MerkleTreeNode merkle_tree[30];
}BitcoinBlock;

void dsha(void* message, unsigned int len, void* digest);

void construct_target(int d, void* target_storage);

int is_good_block(BitcoinHeader* header, const char* target);

void merkle_hash(void* a, void* b, void* digest);

/* void update_merkle_root(BitcoinBlock* block);

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

void attach_block(BitcoinBlock* genesis, BitcoinBlock* new_block); */

void initialize_block(BitcoinBlock* block, int difficulty);

void set_data_node(BitcoinBlock* block, int index, int length, char* data);

void add_data_node(BitcoinBlock* block, int length, char* data);

void update_merkle_root(BitcoinBlock* block);

int get_block_info(char* name, char* next_block_name_storage, char* block_hash_storage, BitcoinBlock* block_storage);

int get_next_block_name(char* name, char* next_name);

int get_block_hash(char* name, char* digest);

int get_block_data(char* name, BitcoinBlock* block);

int get_blockchain_info(char* genesis, char* last_block_name_storage, char* last_block_hash_storage, BitcoinBlock* last_block_storage);

int get_blockchain_length(char* name);

int get_last_block_name(char* name, char* last_name);

int get_last_block_hash(char* name, char* digest);

int get_last_block_data(char* name, BitcoinBlock* block);

int attach_block(char* genesis, BitcoinBlock* new_block, char* new_block_name_storage);

int unlink_shared_memories(char* name, char* name_failure);

int write_block_in_shm(char* name, BitcoinBlock* block);

#endif