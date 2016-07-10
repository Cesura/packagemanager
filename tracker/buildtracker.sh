#!/bin/bash
gcc tracker.c -o tracker -L/usr/lib64/mysql  -lmysqlclient -lpthread -lz -lm -ldl -lssl -lcrypto
