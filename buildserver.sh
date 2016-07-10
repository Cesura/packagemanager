#!/bin/bash
gcc config.c utils.c server.c -o server -lconfig -I include/
