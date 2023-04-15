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
#include <data_utils.h>
#include <custom_errors.h>

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

/* // calculate merkle root and copy it to the header
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
} */

void initialize_block(BitcoinBlock* block, int difficulty){
    // There aren't pointers that might or might not be null this time...
    // but length still have to be wiped at the very least
    memset(block, 0, sizeof(BitcoinBlock));
    // should be clean now lol
    block->header.version=4;
    block->header.difficulty=difficulty;
    block->header.timestamp=time(NULL);
}

void set_data_node(BitcoinBlock* block, int index, int length, char* data){
    block->merkle_tree[index].length=length;
    memcpy(
        block->merkle_tree[index].data,
        data,
        length
    );
}

void add_data_node(BitcoinBlock* block, int length, char* data){
    set_data_node(block, block->tree_length, length, data);
    block->tree_length++;
}

void update_merkle_root(BitcoinBlock* block){
    char d[30][32];
    int this_layer_length=block->tree_length;
    int length_is_odd=this_layer_length%2;
    int next_layer_length=this_layer_length/2+length_is_odd;
    for(int i=0; i<this_layer_length; i++){
        dsha(block->merkle_tree[i].data, block->merkle_tree[i].length, d[i]);
    }
    
    while(this_layer_length>1){
        for(int i=0; i<next_layer_length; i++){
            if(i==next_layer_length-1 && length_is_odd){
                merkle_hash(d[2*i],d[2*i],d[i]);
            }else{
                merkle_hash(d[2*i],d[2*i+1],d[i]);
            }
        }
        this_layer_length=next_layer_length;
        length_is_odd=this_layer_length%2;
        next_layer_length=this_layer_length/2+length_is_odd;
    }
    
    memcpy(block->header.merkle_root, d[0], 32);
}

// Get a block's info based on its name.
// It's slightly hard to use, so it's recommended to use a wrapper for this
// function.
// params
//  *name: name of the shared memory for this block.
//  *next_block_name_storage: if not NULL, copy the next block's name here.
//  *block_hash_storage: if not NULL, copy this block's header hash here.
//  *block_storage: if not NULL, copy the entire block here. Note that this copy
//        is not mapped from the shared memory, so modifying its value does not
//        affect the shared memory.
// return
//  On success, return 0.
//  On error, return a negative number, representing the error's type:
//  E_CUSTOM_NAMETOOLONG: The provided name exceeds 99 characters long.
//  E_CUSTOM_INVALIDSHMNAME: The provided name isn't in the correct format.
//  E_CUSTOM_SHMOPEN: An error occurred in a shm_open() call; check errno.
//  E_CUSTOM_MMAP: An error occurred in a mmap() call; check errno.
//  
int get_block_info(char* name, char* next_block_name_storage, char* block_hash_storage, BitcoinBlock* block_storage){
    if(strnlen(name,100)>99){
        return E_CUSTOM_NAMETOOLONG;
    }
    if(!is_valid_block_shm_name(name)){
        return E_CUSTOM_INVALIDSHMNAME;
    }
    int fd;
    BitcoinBlock* block;
    
    fd=shm_open(name, O_RDWR, 0666);
    if(fd==-1){
        return E_CUSTOM_SHMOPEN;
    }
    block=mmap(NULL, sizeof(BitcoinBlock), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(block==MAP_FAILED){
        return E_CUSTOM_MMAP;
    }
    if(next_block_name_storage!=NULL){
        strncpy(next_block_name_storage, block->next_block, 100);
    }
    if(block_hash_storage!=NULL){
        dsha(&(block->header), sizeof(BitcoinHeader), block_hash_storage);
    }
    if(block_storage!=NULL){
        memcpy(block_storage, block, sizeof(BitcoinBlock));
    }
    close(fd);
    //shm_unlink(name);
    munmap(block, sizeof(BitcoinBlock));
    return 0;
}

int get_next_block_name(char* name, char* next_name){
    return get_block_info(name, next_name, NULL, NULL);
}

int get_block_hash(char* name, char* digest){
    return get_block_info(name, NULL, digest, NULL);
}

int get_block_data(char* name, BitcoinBlock* block){
    return get_block_info(name, NULL, NULL, block);
}

int get_blockchain_info(char* genesis, char* last_block_name_storage, char* last_block_hash_storage, BitcoinBlock* last_block_storage){
    if(strnlen(genesis,100)>99){
        return E_CUSTOM_NAMETOOLONG;
    }
    if(!is_valid_block_shm_name(genesis)){
        return E_CUSTOM_INVALIDSHMNAME;
    }
    int fd;
    BitcoinBlock* block;
    char name[100];
    char name2[100];
    strcpy(name, genesis);
    int count=0;
    while(1){
        fd=shm_open(name, O_RDWR, 0666);
        if(fd==-1){
            return E_CUSTOM_SHMOPEN;
        }
        block=mmap(NULL, sizeof(BitcoinBlock), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if(block==MAP_FAILED){
            return E_CUSTOM_MMAP;
        }
        count++;
        if(block->next_block[0]==0){
            // no next block
            break;
        }
        if(strnlen(block->next_block, 100)>99){
            return E_CUSTOM_NAMETOOLONG;
        }
        if(!is_valid_block_shm_name(block->next_block)){
            return E_CUSTOM_INVALIDSHMNAME;
        }
        strcpy(name2, block->next_block);
        close(fd);
        //shm_unlink(name);
        munmap(block, sizeof(BitcoinBlock));
        strcpy(name, name2);
    }
    if(last_block_name_storage!=NULL){
        strcpy(last_block_name_storage, name);
    }
    if(last_block_hash_storage!=NULL){
        dsha(&(block->header), sizeof(BitcoinHeader), last_block_hash_storage);
    }
    if(last_block_storage!=NULL){
        memcpy(last_block_storage, block, sizeof(BitcoinBlock));
    }
    close(fd);
    //shm_unlink(name);
    munmap(block, sizeof(BitcoinBlock));
    return count;
}

int get_blockchain_length(char* name){
    return get_blockchain_info(name, NULL, NULL, NULL);
}

int get_last_block_name(char* name, char* last_name){
    return get_blockchain_info(name, last_name, NULL, NULL);
}

int get_last_block_hash(char* name, char* digest){
    return get_blockchain_info(name, NULL, digest, NULL);
}

int get_last_block_data(char* name, BitcoinBlock* block){
    return get_blockchain_info(name, NULL, NULL, block);
}

int attach_block(char* genesis, BitcoinBlock* new_block, char* new_block_name_storage){
    if(strnlen(genesis, 100)>99){
        return E_CUSTOM_NAMETOOLONG;
    }
    if(!is_valid_block_shm_name(genesis)){
        return E_CUSTOM_INVALIDSHMNAME;
    }
    char last_name[100];
    BitcoinBlock last_block;
    int rv;
    rv=get_blockchain_info(genesis, last_name, NULL, &last_block);
    if(rv<0){
        return rv;
    }
    if(!is_valid_block_shm_name(last_name)){
        return E_CUSTOM_INVALIDSHMNAME;
    }
    char new_block_hash[32];
    char new_block_name[100];
    dsha(&(new_block->header), sizeof(BitcoinHeader), new_block_hash);
    construct_shm_name(new_block_hash, new_block_name);
    
    strcpy(last_block.next_block, new_block_name);
    strcpy(new_block->previous_block, last_name);
    strcpy(new_block_name_storage, new_block_name);
    
    int fd;
    BitcoinBlock* block;
    
    fd=shm_open(last_name, O_RDWR, 0666);
    if(fd==-1){
        return E_CUSTOM_SHMOPEN;
    }
    block=mmap(NULL, sizeof(BitcoinBlock), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(block==MAP_FAILED){
        return E_CUSTOM_MMAP;
    }
    memcpy(block, &last_block, sizeof(BitcoinBlock));
    close(fd);
    //shm_unlink(last_name);
    munmap(block, sizeof(BitcoinBlock));
    return 0;
}

int unlink_shared_memories(char* name, char* name_failure){
    if(strnlen(name, 100)>99){
        return E_CUSTOM_NAMETOOLONG;
    }
    if(!is_valid_block_shm_name(name)){
        return E_CUSTOM_INVALIDSHMNAME;
    }
    char name1[100];
    char name2[100];
    char name3[100];
    int rv;
    strcpy(name1, name);
    // copying this pre-error so that I can immediately return without
    // potentially updating errno
    strcpy(name_failure, name1);
    rv=get_next_block_name(name1, name2);
    if(rv<0)return rv;
    if(name2[0]==0){
        // there is no second block
        shm_unlink(name1);
        return 0;
    }
    // there is supposedly a second block, but name1 can't be unlinked yet
    // has to verify name2 is actually a valid block
    while(1){
        rv=get_next_block_name(name2, name3);
        // N.B. if it fails here, it's either name2's fault or name2's shm's fault
        // it's definitely not name3's fault
        if(rv<0)return rv;
        // So name2 is good, unlinking name1
        shm_unlink(name1);
        if(name3[0]==0){
            // there is no name3, name2 is last block, unlink it
            shm_unlink(name2);
            return 0;
        }
        strcpy(name1, name2);
        strncpy(name2, name3, 100);
        strcpy(name_failure, name1);
    }
}


int write_block_in_shm(char* name, BitcoinBlock* block){
    if(strnlen(name, 100)>99){
        return E_CUSTOM_NAMETOOLONG;
    }
    if(!is_valid_block_shm_name(name)){
        return E_CUSTOM_INVALIDSHMNAME;
    }
    int fd=shm_open(name, O_CREAT|O_RDWR, 0666);
    if(fd==-1){
        return E_CUSTOM_SHMOPEN;
    }
    int rv=ftruncate(fd, sizeof(BitcoinBlock));
    if(rv==-1){
        return E_CUSTOM_FTRUNCATE;
    }
    BitcoinBlock* b=mmap(NULL, sizeof(BitcoinBlock), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(b==MAP_FAILED){
        return E_CUSTOM_MMAP;
    }
    memcpy(b, block, sizeof(BitcoinBlock));
    
    close(fd);
    munmap(b, sizeof(BitcoinBlock));
    return 0;
}




