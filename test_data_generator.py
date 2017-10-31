#!/usr/local/bin/python3
# -*- coding: utf-8 -*-
##
## DMxxxx Advanced Algorithms
## TSP
##
## Test Data Generation
## Oct. 31, 2017
##

import random
import argparse

parser = argparse.ArgumentParser(prog='TSP Test Data Generation') # usage='Some usage', description='Some description'
parser.add_argument('--min', help='Min value of the coordinate', default=10, type=int)
parser.add_argument('--max', help='Max value of the coordinate', default=200, type=int)
parser.add_argument('-n', '--num', help='Amount of points to generate', default=50, type=int)
args = parser.parse_args()

print(args.num)

for i in range(args.num):
	x = str(round(random.uniform(args.min, args.max), 2))
	y = str(round(random.uniform(args.min, args.max), 2))
	print("{} {}".format(x,y))
