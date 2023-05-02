#!/bin/bash

for i in {1..150}
do
    ./client $((($i % 3 + 1)))  &
done
#>/dev/null