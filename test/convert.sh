#!/bin/bash

count=10000
for x in {1..146}; do
  for y in {1..176}; do
    convert Camille_Pissarro_-_The_Church_at_Eragny_-_Walters_372653.jpg -crop 30x30+${x}0+${y}0 -resize 90x90 image-$count.jpg
    (( count++ ))
  done
done
