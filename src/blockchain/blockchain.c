/**
 * 
 * Author: Filippo Scaramuzza
 * License: MIT
 * 
 * This file contains the implementation of the blockchain data structure.
 * 
 * To compile:
 * gcc -o blockchain blockchain.c -lcrypto
 * */

#include "blockchain.h"
#include "utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/***********************/
/*  UTILITY FUNCTIONS  */
/***********************/

// Blockchain to json
char *blockchain_to_json(blockchain_t *blockchain) {
    // Allocate memory for string to allocate all the blocks
    char *json;
    json = (char *) malloc(sizeof(char) * 1000 * blockchain->length);

    strcat(json, "[");
    // Iterate over the blocks
    for (int i = 0; i < blockchain->length; i++) {
        // Add the block to the string
        strcat(json, block_to_json(blockchain->chain[i]));
        // Add a comma if it is not the last block
        if (i != blockchain->length - 1) {
            strcat(json, ",");
        }
    }
    // Close the json string
    strcat(json, "]");

    // Add terminator to the string
    // strcat(json, "\0");

    return json;
}

blockchain_t *create_blockchain() {
    blockchain_t *blockchain = (blockchain_t *) malloc(sizeof(blockchain_t));
    blockchain->chain = (block_t **) malloc(sizeof(block_t *));
    blockchain->chain[0] = get_genesis_block();

    blockchain->length = 1;

    return blockchain;
}

block_t *add_block(blockchain_t *blockchain, char *data) {
    block_t *new_block = mine_block((blockchain->chain[blockchain->length - 1]), data);
    blockchain->chain = (block_t **) realloc(blockchain->chain, sizeof(block_t *) * (blockchain->length + 1));
    blockchain->chain[blockchain->length] = new_block;
    blockchain->length++;

    return new_block;
}

bool is_chain_valid(block_t **chain, int length) {
    if(chain[0] != get_genesis_block()) { // Genesis block must be the first block
        return FALSE;
    }

    for(int i = 1; i < length; i++) {
        block_t *block = chain[i];
        block_t *last_block = chain[i-1];

        // Check if the block previous hash is the hash of the previous block
        if(strcmp(block->previous_hash, get_hash(last_block)) != 0) {
            return FALSE;
        }
    }

    return TRUE;
}

void replace_chain(block_t **new_chain, int new_length, blockchain_t *blockchain) {
    if(new_length > blockchain->length && is_chain_valid(new_chain, new_length)) {
        free(blockchain->chain);
        blockchain->chain = new_chain;
        blockchain->length = new_length;
    }
}