#!/bin/bash
gcc sprite2bmp.cpp -o sprite2bmp -lstdc++ && ./sprite2bmp && gwenview sprite.bmp
