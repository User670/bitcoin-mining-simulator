#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include <inttypes.h>
#include <fcntl.h>

#include <sha2.h>
#include <bitcoin_utils.h>

// requires `sha2.c`, `sha2.h`
// computes double SHA - that is, SHA twice - for a given piece of input.
// params
//  *message: pointer to the data to be hashed
//  len: how long, in bytes, the input is
//  *digest: pointer to a 32-byte buffer to store the hash
// return
//  void
void dsha(void* message, unsigned int len, void* digest){
    sha256(message, len, digest);
    sha256(digest, 32, digest);
}

// converts a "difficulty" value (a 32-bit integer) to a target hash that can be
// compared against
// params
//  d: the difficulty value
//  *target_storage: pointer to a 32-byte buffer to store the target
// return
//  void
void construct_target(int d, void* target_storage){
    // first two hex digits in the difficulty is the exponent
    // last 6 are the coefficient
    // target = coefficient * 2^(8*(exponent-3))
    // which kinda means place the coefficient (exponent-3) bytes from the right, when written it big-endian
    // this implementation is likely flawed but should work for this purpose
    int coefficient=d%0x1000000;
    int exponent=d>>24;
    int placement_position=32-exponent;
    memset(target_storage, 0, 32);
    memset(target_storage+placement_position, coefficient>>16, 1);
    memset(target_storage+placement_position+1, (coefficient>>8)%0x100, 1);
    memset(target_storage+placement_position+2, coefficient%0x100, 1);
}

// checks whether a block is good.
// @params
//  *block: pointer to a BitcoinHeader
//  *target: pointer to a 32-byte buffer containing the target
// @return
//  1 or 0, to be interpreted as bool, whether the header's hash is below target
int is_good_block(BitcoinHeader* header, const char* target){
    char hash_storage[32];
    dsha(header, sizeof(BitcoinHeader), hash_storage);
    if (memcmp(hash_storage, target, 32)<0){
        return 1;
    }else{
        return 0;
    }
}

// calculate hash of two hashes combined.
// params
//  *a: pointer to a 32-byte buffer holding the first hash
//  *b: pointer to a 32-byte buffer holding the second hash
//  *digest: pointer to a 32-byte buffer to store the result
// returns
//  void
void merkle_hash(void* a, void* b, void* digest){
    char s[64];
    memcpy(&s, a, 32);
    memcpy((&s[32]), b, 32);
    dsha(s, 64, digest);
}

// calculate merkle root and copy it to the header
// params
//  *block: pointer to a block
// return
//  void
void update_merkle_root(BitcoinBlock* block){
    // has to take a pointer
    // taking the block itself makes C duplicate the value for the
    // function, and unable to modify it outside
    calculate_merkle_root_top_down(block->merkle_tree);
    memcpy(block->header.merkle_root,block->merkle_tree->hash,32);
}

// (might work with v3)
// Recursively calculate all hashes in a merkle tree.
// params
//  *node: pointer to the root node
// returns
//  void
void calculate_merkle_root_top_down(MerkleTreeHashNode* node){
    if(node->data==NULL){
        // has left and maybe right hash nodes as children
        // calculate hash of their hashes combined
        if(node->left==NULL){
            perror("calculate_merkle_root_top_down: A non-data node of a merkle tree node has to have a left child.");
            exit(1);
        }else{
            calculate_merkle_root_top_down(node->left);
        }
        if(node->right==NULL){
            // when lacking a right child, left's hash is repeated
            merkle_hash(node->left->hash,node->left->hash,node->hash);
        }else{
            calculate_merkle_root_top_down(node->right);
            merkle_hash(node->left->hash,node->right->hash,node->hash);
        }
    }else{
        // has a data node as child
        // calculate hash of that data
        dsha(node->data->data, node->data->length, node->hash);
    }
}


/*
TODO for bitcoin and merkle tree structure:

- construct from a list of transactions
- construct from random transactions
- add a transaction

- recursively free memory used by a block
- recursively free memory used by a blockchain?
*/

// count how many transactions are in a merkle tree
// assumes tree is valid - specifically, a node with a data node doesn't have
// children, and a node has a left child before a right child.
// params
//  *tree: root of the merkle tree
// return
//  number of transactions
int count_transactions(MerkleTreeHashNode* tree){
    if(tree==NULL)return 0;
    if(tree->data!=NULL){
        return 1;
    }
    int n=0;
    if(tree->left==NULL){
        return n;
    }//else...
    n+=count_transactions(tree->left);
    if(tree->right==NULL){
        return n;
    }//else...
    n+=count_transactions(tree->right);
    return n;
}

// determines whether the tree is full (and thus need a new layer when inserting
// a new transaction).
// params
//  *tree: root of the merkle tree
// return
//  1 or 0 (bool), whether the tree is full
int tree_is_full(MerkleTreeHashNode* tree){
    if(tree==NULL)return 1; // yeah technically
    // returns true if every non-leaf has two children, even if
    // tree is uneven
    if(tree->data!=NULL){
        return 1;
    }
    if(tree->left==NULL || tree->right==NULL){
        return 0;
    }
    return tree_is_full(tree->left) && tree_is_full(tree->right);
}

// counts how deep a tree is on the left-most branch.
// params
//  *tree: root of the merkle tree.
// return
//  depth of the tree. A hash node directly into a data node is depth 1. 
int tree_depth(MerkleTreeHashNode* tree){
    if(tree==NULL)return 0;
    // assuming the tree is a good tree, going left is always valid
    // hash directly into a data is depth 1, hash into hash into data is 2, etc
    MerkleTreeHashNode* n=tree;
    int depth=0;
    while(1){
        if(n->data!=NULL){
            depth+=1;
            break;
        }
        if(n->left!=NULL){
            depth+=1;
            n=n->left;
        }else{
            printf("tree_depth: Malformatted tree\n");
            break;
        }
    }
    return depth;
}

// initializes a hash node with 0 hash and NULL pointers.
// Make sure to assign a child or data node to it - a hash node with three NULL
// pointers is not valid in the tree.
// params
//  *node: the hash node
// return
//  void
void initialize_hash_node(MerkleTreeHashNode* node){
    memset(node->hash, 0, 32);
    node->left=NULL;
    node->right=NULL;
    node->data=NULL;
}

// Adds a layer to a merkle tree, by creating a new hash node and putting the
// old tree on the new node's left child pointer.
// params
//  *tree: root node of the merkle tree
// return
//  void
void add_layer(MerkleTreeHashNode* tree){
    MerkleTreeHashNode* new_node=malloc(sizeof(MerkleTreeHashNode));
    initialize_hash_node(new_node);
    // clone the old root to the new node
    new_node->left=tree->left;
    new_node->right=tree->right;
    new_node->data=tree->data;
    // don't care about hash; it'll have to be updated anyway
    // now wipe the root, assign left
    tree->left=new_node;
    tree->right=NULL;
    tree->data=NULL;
}

// Adds a new data node to the tree. Adds layers and new hash nodes as needed.
// Assumes the tree is valid.
// params
//  *tree: root of the merkle tree
//  *node: the data node to add
// return
//  void
void add_data_node(MerkleTreeHashNode* tree, MerkleTreeDataNode* node){
    int index=count_transactions(tree);
    MerkleTreeHashNode* n=tree;
    if(tree_is_full(n)){
        add_layer(tree);
        n=tree;
    }
    // convert `index` to binary (bits saves the digits in reverse)
    // binary digits -> path down the tree (1=R, 0=L)
    int bits[32];
    int digits=0;
    while(index>0){
        bits[digits]=index%2;
        index/=2;
        digits+=1;
    }
    // navigate the tree
    for(int i=digits-1; i>=0; i--){
        if(bits[i]){
            // 1 for right
            if(n->right==NULL){
                n->right=malloc(sizeof(MerkleTreeHashNode));
                initialize_hash_node(n->right);
            }
            n=n->right;
        }else{
            // 0 for right
            if(n->left==NULL){
                n->left=malloc(sizeof(MerkleTreeHashNode));
                initialize_hash_node(n->left);
            }
            n=n->left;
        }
    }
    // attach the data node
    n->data=node;
}

// ###### TEMPORARY
// Adds an array of data nodes to a tree.
// Not for production use because not-individually-malloc'd nodes can't be
// individually (recursively) free'd.
void construct_merkle_tree(BitcoinBlock* block, int transaction_count, MerkleTreeDataNode* data){
    // assumes `data` in array
    // ... might need rework if data is loose (**data??)
    for(int i=0; i<transaction_count; i++){
        add_data_node(block->merkle_tree, data+i);
    }
}

// Frees a data node, and if it has data, free that memory too.
// params
//  *node: the data node
// return
//  void
void free_data_node(MerkleTreeDataNode* node){
    if(node==NULL)return;
    if(node->data!=NULL)free(node->data);
    free(node);
}

// Recursively free all hash nodes and data nodes in a merkle tree.
// params
//  *node: root node of a merkle tree
// return 
//  void
void recursive_free_merkle_tree(MerkleTreeHashNode* node){
    if(node==NULL)return;
    if(node->left!=NULL)recursive_free_merkle_tree(node->left);
    if(node->right!=NULL)recursive_free_merkle_tree(node->right);
    if(node->data!=NULL)free_data_node(node->data);
    free(node);
}

// Recursively free a bitcoin block and its merkle tree.
// params
//  *block: a block
// return
//  void
void recursive_free_block(BitcoinBlock* block){
    if(block==NULL)return;
    if(block->merkle_tree!=NULL)recursive_free_merkle_tree(block->merkle_tree);
    free(block);
}

// Recursively free a blockchain
// params
//  *block: first block in the chain
// return
//  void
void recursive_free_blockchain(BitcoinBlock* block){
    if(block==NULL)return;
    if(block->next_block!=NULL)recursive_free_blockchain(block->next_block);
    recursive_free_block(block);
}

// Initializes a bitcoin block with 0 hashes, current timestamp, NULL pointers,
// and an initialized hash node.
// Will unreference old merkle tree - free them before you initialize.
// params:
//  *block: a block
//  difficulty: difficulty value to be put into the block's header
// return
//  void
void initialize_block(BitcoinBlock* block, int difficulty){
    //initialize header
    block->header.version=4;
    memset(block->header.previous_block_hash, 0, 32);
    memset(block->header.merkle_root, 0, 32);
    block->header.difficulty=difficulty;
    block->header.timestamp=time(NULL);
    block->header.nonce=0;
    
    //initialize pointers
    block->previous_block=NULL;
    block->next_block=NULL;
    
    //initialize tree
    block->merkle_tree=malloc(sizeof(MerkleTreeHashNode));
    initialize_hash_node(block->merkle_tree);
}

// Attach a block to a blockchain, by assigning this block's prev block pointer
// and the chain's old last block's next block pointer.
// params
//  *genesis: the first block of the chain.
//  *new_block: a block to attach.
// return
//  void
void attach_block(BitcoinBlock* genesis, BitcoinBlock* new_block){
    BitcoinBlock* n=genesis;
    while(n->next_block!=NULL){
        n=n->next_block;
    }
    n->next_block=new_block;
    new_block->previous_block=n;
}




