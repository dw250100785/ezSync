#!/bin/bash
g++ -I../../common/curl/include/ -I./ -I../../common/rapidjson/include/ -L../../common/curl/lib/.libs/ -lcurl test.cpp TransferBase.cpp TransferPcs.cpp -g -o test
