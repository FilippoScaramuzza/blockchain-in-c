#include "block.h"

typedef struct blockchain_t {
    block_t **chain;            // List of blocks
    int length;                 // Length of chain
} blockchain_t;

/***********************/
/*  UTILITY FUNCTIONS  */
/***********************/
char *blockchain_to_json(blockchain_t *blockchain_t);

/***********************/
/*    CORE FUNCTIONS   */
/***********************/
blockchain_t *create_blockchain();                          // Create new blockchain
block_t *add_block(blockchain_t *blockchain, char *data);   // Add block to blockchain
