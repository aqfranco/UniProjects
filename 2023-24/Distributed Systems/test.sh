#!/usr/bin/env bash
SER_PATH=${PWD}/ServerR2
CLI_PATH=${PWD}/Client

cd $SER_PATH
konsole --noclose -e mvn compile exec:java -Ddebug &
konsole --noclose -e mvn compile exec:java -Dexec.args="localhost 2002 B TupleSpaces" -Ddebug &
konsole --noclose -e mvn compile exec:java -Dexec.args="localhost 2003 C TupleSpaces" -Ddebug
