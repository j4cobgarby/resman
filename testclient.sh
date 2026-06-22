#!/usr/bin/env bash

echo "Submitting 1 (time), while server idle --> should run right away"
./build/resman time 1 -m "First (time)"
sleep 1

echo "Submitting 2 (command), while server idle --> should run right away"
./build/resman run -m "Second (command)" -- bash -c 'echo Work: ; for i in {1..5} ; do echo -n $i, ; sleep 1 ; done'
echo ""
# Wait until server has detected job 2 completion
sleep 1

echo "Submitting 3 (time), while server idle --> should run right away"
./build/resman time 3 -m "Third (time)"

echo "Submitting 4 (command), while server busy --> should go into queue and run eventually"
./build/resman run -m "Fourth (command)" -- bash -c 'echo Work: ; for i in {1..5} ; do echo -n $i, ; sleep 1 ; done'
echo ""

echo "Submitting 5 (time), but server busy --> should return an error message saying as much"
./build/resman time 1 -m "Fifth (time, but server is busy)"

echo ""
