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
//  *block: pointer to a header
//  difficulty: the difficulty to be written to the header
// @return
//  void
void get_random_header(BitcoinHeader* block, int difficulty){
    block->version=4;
    get_random_hash(&(block->previous_block_hash));
    get_random_hash(&(block->merkle_root));
    block->timestamp=time(NULL);
    block->difficulty=difficulty;
    block->nonce=0;
}

// gives a random bitcoin header.
// @params
//  *block: pointer to a header
//  difficulty: the difficulty to be written to the header
// @return
//  void
void get_random_continuation_header(BitcoinHeader* block, void* prev_blk_hash, int difficulty){
    block->version=4;
    memcpy(&(block->previous_block_hash), prev_blk_hash, 32);
    get_random_hash(&(block->merkle_root));
    block->timestamp=time(NULL);
    block->difficulty=difficulty;
    block->nonce=0;
}

// Initializes a MerkleTreeDataNode with a random transaction.
// Will overwrite what was already in the node.
// Will not free memory for previous data in the node. See above.
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

// Initializes a block with random transactions.
// Will overwrite what was already in the block.
// Will not free memory for previous data in the block. If there is any
// data, free the memory before calling to prevent memory leak.
// params
//  *block: pointer to a block
// return
//  void
void get_random_block_transactions(BitcoinBlockv2* block){
    // random integer between 5 and 29 - # of transactions to generate
    int num_of_transactions=rand()%25+5;
    block->data_count=num_of_transactions;
    block->data=malloc(num_of_transactions*sizeof(MerkleTreeDataNode));
    for(int i=0; i<num_of_transactions; i++){
        get_random_transaction((block->data)+i);
    }
}




// Dump transactions in a block to a file descriptor.
// params
//  fd: a file descriptor
//  block: a block.
// return
//  void
void dump_transactions(int fd, BitcoinBlockv2 block){
    /*
    // write magic number "OSBT" (OS Bitcoin Transactions)
    write(fd, "OSBT", 4);
    */
    // write # of transactions
    write(fd, &(block.data_count), sizeof(int));
    
    // for each transaction in block...
    for(int i=0; i<block.data_count; i++){
        // write length
        write(fd, &((block.data+i)->length), sizeof(int));
        // write data
        write(fd, (block.data+i)->data, (block.data+i)->length);
    }
    close(fd);
}


// Read a file descriptor for transactions, and load it into a block.
// Does not update merkle root - do it yourself after the read.
// params
//  fd: a file descriptor
//  *block: pointer to a block
// return
//  void
void load_transactions(int fd, BitcoinBlockv2* block){
    /*
    // read and compare magic number
    char magic_number[4];
    read(fd, &magic_number, 4);
    if(memcmp(&magic_number, "OSBT", 4)){
        printf("load_transactions: Wrong magic number\n");
        close(fd);
        exit(1);
    }
    */
    // read data count
    read(fd, &(block->data_count), sizeof(int));
    block->data=malloc(block->data_count*sizeof(MerkleTreeDataNode));
    
    int l;
    // read data
    for(int i=0; i<block->data_count; i++){
        read(fd, &l, sizeof(int));
        ((block->data)+i)->length=l;
        ((block->data)+i)->data=malloc(l);
        read(fd, ((block->data)+i)->data, l);
    }
    
    close(fd);
}

// Dump a block v2, including its transactions, to a file.
// params
//  fd: a file descriptor
//  block: the block
// return
//  void
void dump_block(int fd, BitcoinBlockv2 block){
    write(fd, &block.header, sizeof(BitcoinHeader));
    dump_transactions(fd, block);
}

// Read a blockchain file, and calculate the last block header's hash.
// Relies on the block count metadata to be correct.
// Will get slow if chain gets very long, as it seeks through the blocks
// one by one.
// params
//  fd: a file descriptor
//  *digest: place to store the hash
// return
//  void
void obtain_last_block_hash(int fd, void* digest){
    int block_count;
    int transaction_count;
    int transaction_length;
    
    // read # of blocks
    read(fd, &block_count, sizeof(int));
    
    for(int i=0; i<block_count-1; i++){
        // skip header
        lseek(fd, sizeof(BitcoinHeader), SEEK_CUR);
        // read transaction count
        read(fd, &transaction_count, sizeof(int));
        for(int j=0; j<transaction_count; j++){
            // read transaction length
            read(fd, &transaction_length, sizeof(int));
            // skip the transaction
            lseek(fd, transaction_length, SEEK_CUR);
        }
    }
    
    BitcoinHeader block;
    // read the remaining header
    read(fd, &block, sizeof(BitcoinHeader));
    // hash the header
    dsha(&block, sizeof(BitcoinHeader), digest);
}

// Get a blockchain file's block count by reading the metadata.
// param
//  fd: a file descriptor
// return
//  number of blocks
int obtain_block_count(int fd){
    int a;
    read(fd, &a, sizeof(int));
    return a;
}

void obtain_last_block_hash_v3(BitcoinBlockv3* genesis, void* digest){
    BitcoinBlockv3* n=genesis;
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

int serialize_data_node_v3(MerkleTreeDataNode* node, int max_size, void* buf){
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

int serialize_merkle_tree_v3(MerkleTreeHashNode* node, int max_size, void* buf){
    int depth=tree_depth_v3(node);
    int count=count_transactions_v3(node);
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
        l=serialize_data_node_v3(h->data, max_size-written_bytes, buf+written_bytes);
        if(l==-1)return -1;
        written_bytes+=l;
    }
    return written_bytes;
}

int serialize_block_v3(BitcoinBlockv3* block, int max_size, void* buf){
    int written_bytes=0;
    int l=0;
    // write header
    l=_serialize_value(&(block->header), sizeof(BitcoinHeader), max_size, buf);
    if(l==-1)return -1;
    written_bytes+=l;
    // write tree
    l=serialize_merkle_tree_v3(block->merkle_tree, max_size-written_bytes, buf+written_bytes);
    if(l==-1)return -1;
    written_bytes+=l;
    return written_bytes;
}


int serialize_blockchain_v3(BitcoinBlockv3* genesis, int max_size, void* buf){
    int block_count=1;
    BitcoinBlockv3* n=genesis;
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
        l=serialize_block_v3(n, max_size-written_bytes, buf+written_bytes);
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

int deserialize_data_node_v3(void* serialized_buf, MerkleTreeDataNode* obj){
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

int deserialize_merkle_tree_v3(void* serialized_buf, MerkleTreeHashNode* obj){
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
        l=deserialize_data_node_v3(serialized_buf+read_bytes, n);
        if(l==-1)return -1;
        read_bytes+=l;
        add_data_node_v3(obj, n);
    }
    return read_bytes;
}

int deserialize_block_v3(void* serialized_buf, BitcoinBlockv3* block){
    int read_bytes=0;
    int l=0;
    // load header
    memcpy(&(block->header), serialized_buf, sizeof(BitcoinHeader));
    read_bytes+=sizeof(BitcoinHeader);
    // load tree
    l=deserialize_merkle_tree_v3(serialized_buf+read_bytes, block->merkle_tree);
    if(l==-1)return -1;
    read_bytes+=l;
    return read_bytes;
}

int deserialize_blockchain_v3(void* serialized_buf, BitcoinBlockv3* genesis){
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
    BitcoinBlockv3* b;
    for(int i=0; i<length; i++){
        if(i==0)b=genesis;
        else{
            b=malloc(sizeof(BitcoinBlockv3));
            b->merkle_tree=malloc(sizeof(MerkleTreeHashNode));
        }
        l=deserialize_block_v3(serialized_buf+read_bytes, b);
        if(l==-1)return -1;
        read_bytes+=l;
        if(i!=0)attach_block_v3(genesis, b);
    }    
    return read_bytes;
}




