#!/usr/local/bin/python3
# -*- coding: utf-8 -*-
##
## DMxxxx Advanced Algorithms
## TSP
##
## Parameter Optimization
## Oct. 30, 2017
##

import os
import subprocess
import argparse
import glob
import multiprocessing as mp
from numpy import linspace


DEF_EXE_PATH = "./TSP"
DEF_INPUT_FOLDER = "./in/"

parser = argparse.ArgumentParser(prog='TSP Parameter Optimization', usage='Some usage', description='Some description')
parser.add_argument('-e', '--exepath',   help='path to the executable', default=DEF_EXE_PATH, type=str)
parser.add_argument('-i', '--inputfolder',   help='path to the folder with the input files', default=DEF_INPUT_FOLDER, type=str)
args = parser.parse_args()

exe_path = args.exepath
input_folder = args.inputfolder
input_filepaths = glob.glob(input_folder+"/*")

print("Found {} input files".format(len(input_filepaths)))



def bf_init(results_arg):
    global results
    results = results_arg

def bf_run(params):

	input_filepaths = params["input_filepaths"]
	total = 0

	for fp in input_filepaths:

		cmd = "{exe_path} {noise_ratio} {noise_period} {threeopt_threshold} < {input_filepath}".format(
			exe_path = params["exe_path"],
			input_filepath = fp,
			noise_ratio = params["noise_ratio"],
			noise_period = params["noise_period"],
			threeopt_threshold = params["threeopt_threshold"]
			)
		res = subprocess.run(cmd, shell=True, stdout=subprocess.PIPE)
		res = int(res.stdout.decode('utf-8'))
		total += res

	del params["exe_path"]
	del params["input_filepaths"]
	params["total_length"] = res
	results.append(params)
	print("Worker -> i: {}, {}".format(len(results), params))

def parallel_bf():

	noise_ratio_ls = linspace(0.5, 3.0, num=2)
	noise_period_ls = linspace(1.0, 10.0, num=10) # Content of this should be integers (1+max-min divisible by num)
	threeopt_threshold_ls = linspace(30.0, 30.0, num=1) # Same for this one

	jobs = []
	for nr in noise_ratio_ls:
		for np in noise_period_ls:
			for tt in threeopt_threshold_ls:
				jobs.append({
							"exe_path": exe_path,
							"noise_ratio": nr,
							"noise_period": np,
							"threeopt_threshold": tt,
							"input_filepaths": input_filepaths
						})

	jobs_count = len(jobs)
	print("Total jobs: {}".format(jobs_count))
	
	manager = mp.Manager()
	results = manager.list()

	# No number of forks specified => use all available CPUs
	pool = mp.Pool(initializer=bf_init, initargs=(results,)) 
	pool.map(bf_run, jobs)

	# Sort by total_length
	results = sorted(results, key = lambda res : res["total_length"])

	best_res = results[0]
	best_len = best_res["total_length"]
	shared_count = len(list(filter(lambda res : res["total_length"] == best_len, results)))-1

	print()
	print("Best result: {}".format(best_res))
	print("Score shared with {} other combinations of parameters ({}%)".format(shared_count, round(100.0*shared_count/(jobs_count-1), 2)))


## Old code, use parallelized BF
def brute_force():

	noise_ratio_ls = linspace(0.5, 3.0, num=20)
	noise_period_ls = linspace(1.0, 10.0, num=10)
	threeopt_threshold_ls = linspace(30.0, 30.0, num=1)

	total_iters = len(noise_ratio_ls) * len(noise_period_ls) * len(threeopt_threshold_ls)
	print("Total iterations: {}".format(total_iters))

	best_total = 0
	best_nr = 0
	best_np = 0
	best_tt = 0

	for nr in noise_ratio_ls:
		for np in noise_period_ls:
			for tt in threeopt_threshold_ls:

				total = 0
				
				for fp in input_filepaths:

					res = os.system("{exe_path} {noise_ratio} {noise_period} {threeopt_threshold} < {input_filepath}".format(
							exe_path = exe_path,
							input_filepath = fp,
							noise_ratio = nr,
							noise_period = np,
							threeopt_threshold = tt
						))

					total += res

				if total >= best_total:
					best_total = total
					best_nr = nr
					best_np = np
					best_tt = tt

	print("Max score: {}, params: ({},{},{})", best_total, best_nr, best_np, best_tt)


parallel_bf()
