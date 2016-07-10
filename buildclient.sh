#!/bin/bash
gcc config.c utils.c checksum.c client.c -o client -lconfig -lm -lssl -lcrypto -g -I include/
