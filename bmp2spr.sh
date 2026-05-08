#!/bin/bash
gcc bmp2sprite.cpp -o bmp2sprite -lstdc++ && ./bmp2sprite && cat sprite.h
