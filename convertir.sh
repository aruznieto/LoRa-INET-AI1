#!/bin/bash

sed -i 's/ /_/g' names.txt

for file in *; do mv "$file" `echo $file | tr ' ' '_'`; done 
