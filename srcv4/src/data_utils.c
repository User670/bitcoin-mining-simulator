#include <bitcoin_utils.h>
#include <debug_utils.h>
#include <custom_errors.h>
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

// *adds* random transactions to the block - preferably use this on an empty
// block.
// params
//  *block: the block
// return
//  void
void randomize_block(BitcoinBlock* block){
    // random integer between 5 and 29 - # of transactions
    int length=rand()%25+5;
    MerkleTreeDataNode* n;
    for(int i=0; i<length; i++){
        n=malloc(sizeof(MerkleTreeDataNode));
        get_random_transaction(n);
        add_data_node(block->merkle_tree, n);
    }
    update_merkle_root(block);
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

// Obtain the nonce of the lash block in a chain.
// params
//  *genesis: the first block of a blockchain
// return
//  void
int obtain_last_nonce(BitcoinBlock* genesis){
    BitcoinBlock* n=genesis;
    while(n->next_block!=NULL){
        n=n->next_block;
    }
    return n->header.nonce;
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
int _serialize_value(void* value, int size, int max_size, void* buf, int* err){
    if(size>max_size){
        if(err!=NULL)*err=E_CUSTOM_NOMEM;
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
int serialize_data_node(MerkleTreeDataNode* node, int max_size, void* buf, int* err){
    int written_bytes=0;
    int l=0;
    // write length
    l=_serialize_value(&(node->length), sizeof(int), max_size, buf, err);
    if(l==-1)return -1;
    written_bytes+=l;
    // write data
    l=_serialize_value(node->data, node->length, max_size-written_bytes, buf+written_bytes, err);
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
int serialize_merkle_tree(MerkleTreeHashNode* node, int max_size, void* buf, int* err){
    int depth=tree_depth(node);
    int count=count_transactions(node);
    int written_bytes=0;
    int l=0;
    // write transaction count
    l=_serialize_value(&count, sizeof(int), max_size, buf, err);
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
                    if(err!=NULL)*err=E_CUSTOM_BADTREE;
                    return -1;
                }else{
                    h=h->right;
                }
            }else{
                if(h->left==NULL){
                    if(err!=NULL)*err=E_CUSTOM_BADTREE;
                    return -1;
                }else{
                    h=h->left;
                }
            }
        }
        if(h->data==NULL){
            if(err!=NULL)*err=E_CUSTOM_BADTREE;
            return -1;
        }
        l=serialize_data_node(h->data, max_size-written_bytes, buf+written_bytes, err);
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
int serialize_block(BitcoinBlock* block, int max_size, void* buf, int* err){
    int written_bytes=0;
    int l=0;
    // write header
    l=_serialize_value(&(block->header), sizeof(BitcoinHeader), max_size, buf, err);
    if(l==-1)return -1;
    written_bytes+=l;
    // write tree
    l=serialize_merkle_tree(block->merkle_tree, max_size-written_bytes, buf+written_bytes, err);
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
int serialize_blockchain(BitcoinBlock* genesis, int max_size, void* buf, int* err){
    int block_count=1;
    BitcoinBlock* n=genesis;
    while(n->next_block!=NULL){
        n=n->next_block;
        block_count++;
    }
    int written_bytes=0;
    int l=0;
    // write block count
    l=_serialize_value(&block_count, sizeof(int), max_size, buf, err);
    if(l==-1)return -1;
    written_bytes+=l;
    // write each block
    n=genesis;
    while(1){
        l=serialize_block(n, max_size-written_bytes, buf+written_bytes, err);
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
int deserialize_data_node(MerkleTreeDataNode* obj, void* serialized_buf, int* err){
    // load length
    memcpy(&(obj->length), serialized_buf, sizeof(int));
    if(obj->length<=0){
        if(err!=NULL)*err=E_CUSTOM_BADDATALEN;
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
int deserialize_merkle_tree(MerkleTreeHashNode* obj, void* serialized_buf, int* err){
    // load length
    int read_bytes=0;
    int l=0;
    int transaction_count;
    memcpy(&transaction_count, serialized_buf, sizeof(int));
    read_bytes+=sizeof(int);
    if(transaction_count<=0){
        if(err!=NULL)*err=E_CUSTOM_BADTREELEN;
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
        l=deserialize_data_node(n, serialized_buf+read_bytes, err);
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
int deserialize_block(BitcoinBlock* block, void* serialized_buf, int* err){
    int read_bytes=0;
    int l=0;
    // load header
    memcpy(&(block->header), serialized_buf, sizeof(BitcoinHeader));
    read_bytes+=sizeof(BitcoinHeader);
    // load tree
    l=deserialize_merkle_tree(block->merkle_tree, serialized_buf+read_bytes, err);
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
//            a root hash node.
// returns
//  On success, return number of bytes read.
//  On error, return -1.
int deserialize_blockchain(BitcoinBlock* genesis, void* serialized_buf, int* err){
    int read_bytes=0;
    int l=0;
    // load chain length
    int length=0;
    memcpy(&length, serialized_buf, sizeof(int));
    if(length==0){
        if(err!=NULL)*err=E_CUSTOM_EMPTYCHAIN;
        return -1;
    }else if(length<0){
        if(err!=NULL)*err=E_CUSTOM_BADCHAINLEN;
        return -1;
    }
    read_bytes=sizeof(int);
    BitcoinBlock* b;
    for(int i=0; i<length; i++){
        if(i==0)b=genesis;
        else{
            b=malloc(sizeof(BitcoinBlock));
            initialize_block(b,0);
        }
        l=deserialize_block(b, serialized_buf+read_bytes, err);
        if(l==-1)return -1;
        read_bytes+=l;
        if(i!=0)attach_block(genesis, b);
    }    
    return read_bytes;
}

// Get a dummy genesis block, with the same data as the real Bitcoin genesis block.
// The reason why this exists is to simplify things - initializing the chain with
// a block means code don't need to deal with empty chains.
// (Yes, the dummy block's header hash has zeros on the end, we're not gonna care
// about endianness in this simulator)
// params
//  *block: an initialized block with a merkle hash node already allocated.
// return
//  void
void get_dummy_genesis_block(BitcoinBlock* block){
    block->header.version=1;
    memset(block->header.previous_block_hash, 0, 32);
    // merkle root will be updated later
    block->header.timestamp=1231006505;
    block->header.difficulty=0x1d00ffff;
    block->header.nonce=2083236893;
    
    char _data[]={
            0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
            0xff, 0x4d, 0x04, 0xff, 0xff, 0x00, 0x1d, 0x01,
            0x04, 0x45, 0x54, 0x68, 0x65, 0x20, 0x54, 0x69,
            0x6d, 0x65, 0x73, 0x20, 0x30, 0x33, 0x2f, 0x4a,
            0x61, 0x6e, 0x2f, 0x32, 0x30, 0x30, 0x39, 0x20,
            0x43, 0x68, 0x61, 0x6e, 0x63, 0x65, 0x6c, 0x6c,
            0x6f, 0x72, 0x20, 0x6f, 0x6e, 0x20, 0x62, 0x72,
            0x69, 0x6e, 0x6b, 0x20, 0x6f, 0x66, 0x20, 0x73,
            0x65, 0x63, 0x6f, 0x6e, 0x64, 0x20, 0x62, 0x61,
            0x69, 0x6c, 0x6f, 0x75, 0x74, 0x20, 0x66, 0x6f,
            0x72, 0x20, 0x62, 0x61, 0x6e, 0x6b, 0x73, 0xff,
            0xff, 0xff, 0xff, 0x01, 0x00, 0xf2, 0x05, 0x2a,
            0x01, 0x00, 0x00, 0x00, 0x43, 0x41, 0x04, 0x67,
            0x8a, 0xfd, 0xb0, 0xfe, 0x55, 0x48, 0x27, 0x19,
            0x67, 0xf1, 0xa6, 0x71, 0x30, 0xb7, 0x10, 0x5c,
            0xd6, 0xa8, 0x28, 0xe0, 0x39, 0x09, 0xa6, 0x79,
            0x62, 0xe0, 0xea, 0x1f, 0x61, 0xde, 0xb6, 0x49,
            0xf6, 0xbc, 0x3f, 0x4c, 0xef, 0x38, 0xc4, 0xf3,
            0x55, 0x04, 0xe5, 0x1e, 0xc1, 0x12, 0xde, 0x5c,
            0x38, 0x4d, 0xf7, 0xba, 0x0b, 0x8d, 0x57, 0x8a,
            0x4c, 0x70, 0x2b, 0x6b, 0xf1, 0x1d, 0x5f, 0xac,
            0x00, 0x00, 0x00, 0x00
        };
    
    MerkleTreeDataNode* data=malloc(sizeof(MerkleTreeDataNode));
    data->length=204;
    data->data=malloc(204);
    memcpy(data->data, _data, 204);
    block->merkle_tree->data=data;
    update_merkle_root(block);
}

// Convert a byte array to hex string. This doesn't add the `\0` at the end.
// params
//  *bytes: the bytes to convert.
//  size: how many bytes to convert.
//  *storage: place to store the string.
// return
//  void
void bytes_to_hex_string(void* bytes, int size, void* storage){
    for(int i=0; i<size; i++){
        sprintf(storage+i*2, "%02x", *(unsigned char*)(bytes+i));
    }
}

// Write a blockchain to a file.
// This will write an int of how long the serialized data is in bytes, followed by
// data from serialize_blockchain.
// If serialize_blockchain fails, an int of -1 will be written to indicate a failure.
// params
//  fd: a file descriptor
//  max_size: size of a buffer to temporarily store the serialized data.
//  *block: the genesis block of the blockchain.
// return
//  Returns the value returned by serialize_blockchain.
//  Note that when it returns a size, the actual file is 4 bytes larger.
int write_blockchain_to_file(int fd, int max_size, BitcoinBlock* block){
    char* data=malloc(max_size);
    int size=serialize_blockchain(block, max_size, data, NULL);
    // write byte size metadata, because read() isn't aware of block structure,
    // and needs a numerical number of bytes to write
    // even if size is -1, still write it to indicate a bad blockchain
    write(fd, &size, sizeof(int));
    if(size!=-1){
        write(fd, data, size);
    }
    free(data);
    return size;
}

// Reads a blockchain from a file.
// params
//  fd: a file descriptor.
//  *block: where to store the blockchain.
// return
//  When the data size metadata reads positive, returns the value returned by
//  deserialize_block.
//  When the metadata reads zero or negative, returns -1.
int read_blockchain_from_file(int fd, BitcoinBlock* block){
    int size;
    read(fd, &size, sizeof(int));
    if(size<=0){
        return -1;
    }
    char* data=malloc(size);
    read(fd, data, size);
    int rv=deserialize_blockchain(block, data, NULL);
    return rv;
}



