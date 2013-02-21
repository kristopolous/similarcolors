#!/bin/bash
if [ $# -eq 1 ]; then
  ls *.jpg | awk ' { print "+test/"$0 } ' | nc localhost $1
else
  ls *.jpg | awk ' { print "+'$2':test/"$0 } ' | nc localhost $1
fi
