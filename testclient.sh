#!/usr/bin/env bash

./build/resman run -m "Hello!" -- bash -c 'echo Work: ; for i in {1..20} ; do echo -n $i, ; sleep 2 ; done'
