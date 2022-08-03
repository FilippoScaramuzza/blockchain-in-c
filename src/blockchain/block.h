/**
 * 
 * Author: Filippo Scaramuzza
 * License: MIT
 * 
 * This file contains the definition of the blockchain data structure.
 * 
 * */

/***********************/
/*   DATA STRUCTURES   */
/***********************/

typedef struct block_t {
    int timestamp;          // Time when block was created
    char *previous_hash;    // Hash of previous block
    char *hash;             // Hash of block
    char *data;             // Data stored in block
} block_t;

/***********************/
/*  UTILITY FUNCTIONS  */
/***********************/
char *get_ascii_hash(char *hash);       // Prints hash as a string of ascii characters
char *block_to_json(block_t *block);    // Convert block to string representation for printing
char *get_hash(block_t *block);         // Get hash of block

/***********************/
/*    CORE FUNCTIONS   */
/***********************/
block_t *get_genesis_block();                           // Get genesis block
block_t *mine_block(block_t *last_block, char *data);    // Mine block