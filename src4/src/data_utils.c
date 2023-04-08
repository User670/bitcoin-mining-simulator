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

void obtain_last_block_hash(BitcoinBlock* genesis, void* digest){
    BitcoinBlock* n=genesis;
    while(n->next_block!=NULL){
        n=n->next_block;
    }
    dsha(&(n->header), sizeof(BitcoinHeader), digest);
}


int _serialize_value(void* value, int size, int max_size, void* buf){
    if(size>max_size){
        errno=ENOMEM;
        return -1;
    }
    memcpy(buf, value, size);
    return size;
}

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




