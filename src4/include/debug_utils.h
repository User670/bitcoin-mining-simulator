#pragma once
#include <stdio.h>
#include <time.h>
#include <bitcoin_utils.h>

void debug_print_hex(void* data, int bytes);

void debug_print_hex_line(void* data, int bytes);

void debug_print_header(BitcoinHeader block);