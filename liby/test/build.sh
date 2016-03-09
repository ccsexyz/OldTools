#!/bin/sh

BUILD() {
    while [ $# -gt 0 ]
    do
        gcc -g -o $1 $1".c" ../src/*.c -lpthread -std=gnu99
        shift
    done
}

BUILD echo echo_client chat httpd httpd2
