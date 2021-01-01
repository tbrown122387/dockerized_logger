#!/bin/bash

cd IBJts/samples/Cpp/TestCppClient/
make clean
make
echo "SUCCESSFULLY BUILT IB CLIENT LOGGING APP\n"
echo "NOW RUNNING THE CLIENT LOGGING APP\n"
./emini_logger

