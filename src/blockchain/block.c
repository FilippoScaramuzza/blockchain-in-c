/**
 * 
 * Author: Filippo Scaramuzza
 * License: MIT
 * 
 * This file contains the implementation of the block data structure.
 * 
 * To compile:
 * gcc -o block block.c -lcrypto
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openssl/sha.h>
#include "block.h"

#define SHA256_DIGEST_LENGTH 32

/***********************/
/*  UTILITY FUNCTIONS  */
/***********************/

// Prints hash as a string of ascii characters
char *get_ascii_hash(char *hash)
{
    // Convert hash to string
    char *hash_string = malloc(sizeof(char) * (SHA256_DIGEST_LENGTH * 2 + 1));
    if (!hash_string)
    {
        printf("Error allocating memory for hash string\n");
        exit(1);
    }
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        sprintf(hash_string + (i * 2), "%02hhx", hash[i]);
    }
    hash_string[SHA256_DIGEST_LENGTH * 2] = '\0';

    // Return hash string
    return hash_string;
}

// Convert block to string representation for printing
char *block_to_json(block_t *block)
{
    // Allocate memory for string
    char *json;
    json = (char *) malloc(sizeof(char) * 1000);
    // Create json string
    sprintf(json, "{\"timestamp\":%d,\"previous_hash\":\"%s\",\"hash\":\"%s\",\"data\":\"%s\"}", block->timestamp, block->previous_hash, block->hash, block->data);
    // Return json string
    return json;
}

// Function to get hash of block
char *get_hash(block_t *block)
{
    struct block_header {
        int timestamp;
        char *previous_hash;
        char *data;
    };

    // Allocate memory for hash
    char *hash = malloc(sizeof(char) * SHA256_DIGEST_LENGTH);
    if (!hash)
    {
        printf("Error allocating memory for hash\n");
        exit(1);
    }

    struct block_header *header = malloc(sizeof(struct block_header));
    if (!header)
    {
        printf("Error allocating memory for header\n");
        exit(1);
    }

    header->timestamp = block->timestamp;
    header->previous_hash = block->previous_hash;
    header->data = block->data;

    // Calculate hash with SHA256
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, header, sizeof(struct block_header));
    SHA256_Final((unsigned char *) hash, &ctx);

    // Return hash
    return hash;
}

/***********************/
/*    CORE FUNCTIONS   */
/***********************/

// Function to get the genesis block
block_t *get_genesis_block()
{
    // Allocate memory for block
    block_t *block = malloc(sizeof(block_t));
    if (!block)
    {
        printf("Error allocating memory for block\n");
        exit(1);
    }

    // Set block timestamp
    block->timestamp = 0;

    // Set block previous hash
    block->previous_hash = NULL;

    // Set block hash
    block->hash = NULL;

    // Set block data
    block->data = "Genesis block";

    // Return block
    return block;
}

block_t *mine_block(block_t *last_block, char *data) {
    // Allocate memory for block
    block_t *block = malloc(sizeof(block_t));
    if (!block)
    {
        printf("Error allocating memory for block\n");
        exit(1);
    }

    // Set block timestamp
    block->timestamp = 0;

    // Set block previous hash
    block->previous_hash = last_block->hash;

    // Set block hash
    block->hash = 0;

    // Set block data
    block->data = data;

    // Return block
    return block;
}