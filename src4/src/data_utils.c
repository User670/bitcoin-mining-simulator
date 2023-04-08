#include <bitcoin_utils.h>
#include <sha2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

// gives a random 32-byte hash.
// For simplicity, it just hashes a random integer.
// @params
//  *digest: pointer to a 32-byte buffer to store the hash
// @return
//  void
void get_random_hash(void* digest){
    int a=rand();
    dsha((void*)&a, sizeof(int), digest);
}

// gives a random bitcoin header.
// @params
//  *header: pointer to a header
//  difficulty: the difficulty to be written to the header
// @return
//  void
void get_random_header(BitcoinHeader* header, int difficulty){
    header->version=4;
    get_random_hash(&(header->previous_block_hash));
    get_random_hash(&(header->merkle_root));
    header->timestamp=time(NULL);
    header->difficulty=difficulty;
    header->nonce=0;
}

// gives a random bitcoin header.
// @params
//  *header: pointer to a header
//  difficulty: the difficulty to be written to the header
// @return
//  void
void get_random_continuation_header(BitcoinHeader* header, void* prev_blk_hash, int difficulty){
    header->version=4;
    memcpy(&(header->previous_block_hash), prev_blk_hash, 32);
    get_random_hash(&(header->merkle_root));
    header->timestamp=time(NULL);
    header->difficulty=difficulty;
    header->nonce=0;
}

// Initializes a MerkleTreeDataNode with a random transaction.
// Will overwrite what was already in the node.
// Will not free memory for previous data in the node.
// params:
//  *node: pointer to a node
// return
//  void
void get_random_transaction(MerkleTreeDataNode* node){
    // random integer between 128 and 511 - length of transaction
    int length=rand()%384+128;
    node->length=length;
    node->data=malloc(length);
    for(int i=0; i<length; i++){
        *((node->data)+i)=(char)(rand()%256);
    }
}

// Calculates the hash of the last block's header.
// params
//  *genesis: the first block of a blockchain.
//  *digest: a 32-byte buffer to store the hash.
// return
//  void
void obtain_last_block_hash(BitcoinBlock* genesis, void* digest){
    BitcoinBlock* n=genesis;
    while(n->next_block!=NULL){
        n=n->next_block;
    }
    dsha(&(n->header), sizeof(BitcoinHeader), digest);
}

// ######PRIVATE
// "serialize" a value into a buffer.
// params
//  *value: the value to store.
//  size: the size, in bytes, of the value to store. (use sizeof)
//  max_size: the max size the buffer can handle, in bytes.
//  *buf: the buffer.
// return
//  On success, return the number of bytes written (=size).
//  On error, return -1.
// errors
//  ENOMEM: The buffer doesn't have enough memory to hold the data, as indicated
//          by the max_size parameter.
int _serialize_value(void* value, int size, int max_size, void* buf){
    if(size>max_size){
        errno=ENOMEM;
        return -1;
    }
    memcpy(buf, value, size);
    return size;
}

// Serialize a data node into a buffer as binary.
// A data node is stored as an int (the length of the data) followed by the data.
// Data serialized this way is not portable across systems, since the width and
// endianness of values can vary.
// params
//  *node: the data node to serialize
//  max_size: the max size the buffer can handle, in bytes.
//  *buf: the buffer.
// return
//  On success, return the number of bytes written.
//  On error, return -1.
// errors
//  ENOMEM: The buffer doesn't have enough memory to hold the data, as indicated
//          by the max_size parameter.
int serialize_data_node(MerkleTreeDataNode* node, int max_size, void* buf){
    int written_bytes=0;
    int l=0;
    // write length
    l=_serialize_value(&(node->length), sizeof(int), max_size, buf);
    if(l==-1)return -1;
    written_bytes+=l;
    // write data
    l=_serialize_value(node->data, node->length, max_size-written_bytes, buf+written_bytes);
    if(l==-1)return -1;
    written_bytes+=l;
    return written_bytes;
}

// Serialize a merkle tree into a buffer as binary.
// A tree is stored as an int (number of data nodes it has) followed by this
// many data nodes serialized.
// Data serialized this way is not portable across systems, since the width and
// endianness of values can vary.
// Assumes the tree is valid.
// params
//  *node: the root to the merkle tree to serialize
//  max_size: the max size the buffer can handle, in bytes.
//  *buf: the buffer.
// return
//  On success, return the number of bytes written.
//  On error, return -1.
// errors
//  ENOMEM: The buffer doesn't have enough memory to hold the data, as indicated
//          by the max_size parameter.
//  EINVAL: The merkle tree seems invalid in some way.
int serialize_merkle_tree(MerkleTreeHashNode* node, int max_size, void* buf){
    int depth=tree_depth(node);
    int count=count_transactions(node);
    int written_bytes=0;
    int l=0;
    // write transaction count
    l=_serialize_value(&count, sizeof(int), max_size, buf);
    if(l==-1)return -1;
    written_bytes+=l;
    // traverse tree and write data
    int bits[32];
    int n=0;
    MerkleTreeHashNode* h;
    for(int i=0; i<count; i++){
        h=node;
        n=i;
        for(int d=0; d<depth; d++){
            bits[d]=n%2;
            n/=2;
        }
        for(int d=depth-2; d>=0; d--){
            if(bits[d]){
                if(h->right==NULL){
                    errno=EINVAL;
                    return -1;
                }else{
                    h=h->right;
                }
            }else{
                if(h->left==NULL){
                    errno=EINVAL;
                    return -1;
                }else{
                    h=h->left;
                }
            }
        }
        if(h->data==NULL){
            errno=EINVAL;
            return -1;
        }
        l=serialize_data_node(h->data, max_size-written_bytes, buf+written_bytes);
        if(l==-1)return -1;
        written_bytes+=l;
    }
    return written_bytes;
}

// Serialize a bitcoin block into a buffer as binary.
// A block is stored as the header, in its native binary representation, followed
// by the merkle tree.
// Data serialized this way is not portable across systems, since the width and
// endianness of values can vary.
// params
//  *block: the block to serialize
//  max_size: the max size the buffer can handle, in bytes.
//  *buf: the buffer.
// return
//  On success, return the number of bytes written.
//  On error, return -1.
// errors
//  ENOMEM: The buffer doesn't have enough memory to hold the data, as indicated
//          by the max_size parameter.
//  EINVAL: The merkle tree seems invalid in some way.
int serialize_block(BitcoinBlock* block, int max_size, void* buf){
    int written_bytes=0;
    int l=0;
    // write header
    l=_serialize_value(&(block->header), sizeof(BitcoinHeader), max_size, buf);
    if(l==-1)return -1;
    written_bytes+=l;
    // write tree
    l=serialize_merkle_tree(block->merkle_tree, max_size-written_bytes, buf+written_bytes);
    if(l==-1)return -1;
    written_bytes+=l;
    return written_bytes;
}

// Serialize a blockchain into a buffer as binary.
// A blockchain is stored as an int (number of blocks in the chain) followed by
// this many blocks serialized.
// Data serialized this way is not portable across systems, since the width and
// endianness of values can vary.
// params
//  *genesis: the first block of the blockchain to serialize
//  max_size: the max size the buffer can handle, in bytes.
//  *buf: the buffer.
// return
//  On success, return the number of bytes written.
//  On error, return -1.
// errors
//  ENOMEM: The buffer doesn't have enough memory to hold the data, as indicated
//          by the max_size parameter.
//  EINVAL: A merkle tree in the blockchain seems invalid in some way.
int serialize_blockchain(BitcoinBlock* genesis, int max_size, void* buf){
    int block_count=1;
    BitcoinBlock* n=genesis;
    while(n->next_block!=NULL){
        n=n->next_block;
        block_count++;
    }
    int written_bytes=0;
    int l=0;
    // write block count
    l=_serialize_value(&block_count, sizeof(int), max_size, buf);
    if(l==-1)return -1;
    written_bytes+=l;
    // write each block
    n=genesis;
    while(1){
        l=serialize_block(n, max_size-written_bytes, buf+written_bytes);
        if(l==-1)return -1;
        written_bytes+=l;
        if(n->next_block!=NULL){
            n=n->next_block;
        }else{
            break;
        }
    }
    return written_bytes;
}

// Deserialize a data node.
// Will malloc memory for the data in the node.
// params
//  *serialized_buf: serialized data
//  *obj: a data node to store the data in.
// returns
//  On success, return number of bytes read.
//  On error, return -1.
// errors
//  EINVAL: The length reads zero or negative.
int deserialize_data_node(void* serialized_buf, MerkleTreeDataNode* obj){
    // load length
    memcpy(&(obj->length), serialized_buf, sizeof(int));
    if(obj->length<=0){
        errno=EINVAL;
        return -1;
    }
    // load transaction
    obj->data=malloc(obj->length);
    if(obj->data==NULL)return -1;
    memcpy(obj->data, serialized_buf, obj->length);
    return sizeof(int)+obj->length;
}

// Deserialize a merkle tree, by reading all nodes and reconstructing the tree.
// Will malloc memory for hash nodes and data nodes.
// params
//  *serialized_buf: serialized data
//  *obj: a hash node to be the root of the tree.
// returns
//  On success, return number of bytes read.
//  On error, return -1.
// errors
//  EINVAL: The length of the tree, or a data node, reads zero or negative.
int deserialize_merkle_tree(void* serialized_buf, MerkleTreeHashNode* obj){
    // load length
    int read_bytes=0;
    int l=0;
    int transaction_count;
    memcpy(&transaction_count, serialized_buf, sizeof(int));
    read_bytes+=sizeof(int);
    if(transaction_count<=0){
        errno=EINVAL;
        return -1;
    }
    // initialize the hash node
    obj->left=NULL;
    obj->right=NULL;
    obj->data=NULL;
    // load transactions
    MerkleTreeDataNode* n;
    for(int i=0; i<transaction_count; i++){
        n=malloc(sizeof(MerkleTreeDataNode));
        l=deserialize_data_node(serialized_buf+read_bytes, n);
        if(l==-1)return -1;
        read_bytes+=l;
        add_data_node(obj, n);
    }
    return read_bytes;
}

// Deserialize a bitcoin block.
// Will malloc memory for the merkle tree's non-root nodes.
// params
//  *serialized_buf: serialized data
//  *block: a block to store the data. The block has to be initialized with
//          a root hash node.
// returns
//  On success, return number of bytes read.
//  On error, return -1.
// errors
//  EINVAL: The length of the tree, or a data node, reads zero or negative.
int deserialize_block(void* serialized_buf, BitcoinBlock* block){
    int read_bytes=0;
    int l=0;
    // load header
    memcpy(&(block->header), serialized_buf, sizeof(BitcoinHeader));
    read_bytes+=sizeof(BitcoinHeader);
    // load tree
    l=deserialize_merkle_tree(serialized_buf+read_bytes, block->merkle_tree);
    if(l==-1)return -1;
    read_bytes+=l;
    return read_bytes;
}

// Deserialize a bitcoin block.
// Will malloc memory for the genesis block's tree's non-root nodes, as well as
// all memory for subsequent blocks.
// params
//  *serialized_buf: serialized data
//  *genesis: a block to store the data. The block has to be initialized with
//          a root hash node.
// returns
//  On success, return number of bytes read.
//  On error, return -1.
// errors
//  EINVAL: The length of the chain, a tree, or a data node, reads zero or negative.
int deserialize_blockchain(void* serialized_buf, BitcoinBlock* genesis){
    int read_bytes=0;
    int l=0;
    // load chain length
    int length=0;
    memcpy(&length, serialized_buf, sizeof(int));
    if(length<=0){
        errno=EINVAL;
        return -1;
    }
    read_bytes=sizeof(int);
    BitcoinBlock* b;
    for(int i=0; i<length; i++){
        if(i==0)b=genesis;
        else{
            b=malloc(sizeof(BitcoinBlock));
            b->merkle_tree=malloc(sizeof(MerkleTreeHashNode));
        }
        l=deserialize_block(serialized_buf+read_bytes, b);
        if(l==-1)return -1;
        read_bytes+=l;
        if(i!=0)attach_block(genesis, b);
    }    
    return read_bytes;
}




