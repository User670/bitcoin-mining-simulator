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
// @params
//  *message: pointer to the data to be hashed
//  len: how long, in bytes, the input is
//  *digest: pointer to a 32-byte buffer to store the hash
// @return
//  void
void dsha(void* message, unsigned int len, void* digest){
    sha256(message, len, digest);
    sha256(digest, 32, digest);
}

// converts a "difficulty" value (a 32-bit integer) to a target hash that can be
// compared against
// @params
//  d: the difficulty value
//  *target_storage: pointer to a 32-byte buffer to store the target
// @return
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
    dsha(header, sizeof(BitcoinHeader), &hash_storage);
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
    dsha(&s, 64, digest);
}



void update_merkle_root(BitcoinBlock* block){
    // has to take a pointer
    // taking the block itself makes C duplicate the value for the
    // function, and unable to modify it outside
    calculate_merkle_root_top_down(block->merkle_tree);
    memcpy(&(block->header.merkle_root),&(block->merkle_tree->hash),32);
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
            merkle_hash(&(node->left->hash),&(node->left->hash),&(node->hash));
        }else{
            calculate_merkle_root_top_down(node->right);
            merkle_hash(&(node->left->hash),&(node->right->hash),&(node->hash));
        }
    }else{
        // has a data node as child
        // calculate hash of that data
        dsha(&(node->data->data), node->data->length, &(node->hash));
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

/*

*/
int count_transactions_(MerkleTreeHashNode* node){
    if(node==NULL)return 0;
    if(node->data!=NULL){
        return 1;
    }
    int n=0;
    if(node->left==NULL){
        return n;
    }//else...
    n+=count_transactions(node->left);
    if(node->right==NULL){
        return n;
    }//else...
    n+=count_transactions(node->right);
    return n;
}

int tree_is_full(MerkleTreeHashNode* node){
    if(node==NULL)return 1; // yeah technically
    // returns true if every non-leaf has two children, even if
    // tree is uneven
    if(node->data!=NULL){
        return 1;
    }
    if(node->left==NULL || node->right==NULL){
        return 0;
    }
    return tree_is_full(node->left) && tree_is_full(node->right);
}

int tree_depth(MerkleTreeHashNode* node){
    if(node==NULL)return 0;
    // assuming the tree is a good tree, going left is always valid
    // hash directly into a data is depth 1, hash into hash into data is 2, etc
    MerkleTreeHashNode* n=node;
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

void initialize_hash_node(MerkleTreeHashNode* node){
    memset(node->hash, 0, 32);
    node->left=NULL;
    node->right=NULL;
    node->data=NULL;
}

void add_layer(MerkleTreeHashNode* tree){
    MerkleTreeHashNode* n=tree;
    tree=malloc(sizeof(MerkleTreeHashNode));
    tree->left=n;
}

void add_data_node(MerkleTreeHashNode* tree, MerkleTreeDataNode* node){
    int index=count_transactions(tree);
    MerkleTreeHashNode* n=tree;
    if(tree_is_full(n)){
        add_layer(tree);
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
void construct_merkle_tree(BitcoinBlock* block, int transaction_count, MerkleTreeDataNode* data){
    // assumes `data` in array
    // ... might need rework if data is loose (**data??)
    for(int i=0; i<transaction_count; i++){
        add_data_node(block->merkle_tree, data+i);
    }
}

void free_data_node(MerkleTreeDataNode* node){
    if(node->data!=NULL)free(node->data);
    free(node);
}

void recursive_free_merkle_tree(MerkleTreeHashNode* node){
    if(node->left!=NULL)recursive_free_merkle_tree(node->left);
    if(node->right!=NULL)recursive_free_merkle_tree(node->right);
    if(node->data!=NULL)free_data_node(node->data);
    free(node);
}

void recursive_free_block(BitcoinBlock* block){
    recursive_free_merkle_tree(block->merkle_tree);
    free(block);
}

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

void attach_block(BitcoinBlock* genesis, BitcoinBlock* new_block){
    BitcoinBlock* n=genesis;
    while(n->next_block!=NULL){
        n=n->next_block;
    }
    n->next_block=new_block;
    new_block->previous_block=n;
}




