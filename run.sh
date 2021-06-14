#!/bin/bash

case "$1" in
   "compile")
     g++ --std=gnu++17 region_ident.cpp -L./lib -lqbe -I ./include -D DEBUG -o region_ident -fsanitize=leak
     ;;
   "test")
     for file in tests/*.txt; do
       printf "%s\n" $file
       output_name=$(tr '/' '_' <<<"$file")
       ./region_ident <"$file" >outputs/"$output_name"
     done
     ;;
   "-h"|"--help"|*)
     echo "You have failed to specify what to do correctly."
     echo "Available options:"
     echo '"./run compile" for compiling'
     echo '"./run test" for testing'
     exit 1
     ;;
esac