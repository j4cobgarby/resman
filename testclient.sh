#!/bin/bash

./build/resman run -V -m "Hello!" -- bash -c 'echo Work: ; for i in {1..20} ; do echo -n $i, ; sleep 0.15 ; done'
