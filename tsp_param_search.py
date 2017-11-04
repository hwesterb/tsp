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
import csv
import subprocess
import argparse
import glob
import time
import multiprocessing as mp
from numpy import linspace, append


DEF_EXE_PATH = "./TSP"
DEF_INPUT_FOLDER = "./in/"
TSP_TIME_LIMIT = 2.08 # seconds

parser = argparse.ArgumentParser(prog='TSP Parameter Optimization', usage='Some usage', description='Some description')
parser.add_argument('-e', '--exepath',   	help='path to the executable', default=DEF_EXE_PATH, type=str)
parser.add_argument('-i', '--inputfolder',  help='path to the folder with the input files', default=DEF_INPUT_FOLDER, type=str)
parser.add_argument('-o', '--outputfile',   help='path to the output csv file with results', default=None, type=str)
args = parser.parse_args()

concurrency = mp.cpu_count()
exe_path = args.exepath
output_filepath = args.outputfile
input_folder = args.inputfolder
input_filepaths = glob.glob(input_folder+"/*")

print("Found {} input files".format(len(input_filepaths)))



def results_to_csv(results, csv_path):

	if not csv_path.endswith(".csv"):
		csv_path += ".csv"

	print("\nWriting results to CSV file...")

	with open(csv_path, 'w+', encoding='utf-8') as csvfile:
		writer = csv.writer(
			csvfile,
			delimiter=',',
			quotechar='"',
			quoting=csv.QUOTE_MINIMAL
		)
		for r in results:
			writer.writerow([
				r['noise_ratio'],
				r['noise_period'],
				r['threeopt_threshold'],
				r['backtrack_period'],
				r['double_bridge_period'],
				r['noise_iters_ratio'],
				r['total_length'],
				r['total_time']
			])
	print("Wrote to {}".format(csv_path))



def bf_init(results_arg):
    global results
    results = results_arg

def bf_run(params):

	input_filepaths = params["input_filepaths"]
	total_length = 0
	total_time = 0.0

	for fp in input_filepaths:

		cmd = "{exe_path} {noise_ratio} {noise_period} {threeopt_threshold} {backtrack_period} {double_bridge_period} {noise_iters_ratio}< {input_filepath}".format(
			exe_path = params["exe_path"],
			input_filepath = fp,
			noise_ratio = params["noise_ratio"],
			noise_period = params["noise_period"],
			threeopt_threshold = params["threeopt_threshold"],
			backtrack_period = params["backtrack_period"],
			double_bridge_period = params["double_bridge_period"],
			noise_iters_ratio = params["noise_iters_ratio"]
		)
		tick = time.perf_counter()
		res = subprocess.run(cmd, shell=True, stdout=subprocess.PIPE)
		tock = time.perf_counter()
		length = int(res.stdout.decode('utf-8'))
		total_length += length
		total_time += (tock-tick)

	del params["exe_path"]
	del params["input_filepaths"]
	params["total_length"] = total_length
	params["total_time"] = total_time
	results.append(params)
	print("Job {} -> {}".format(len(results), params))



def parallel_bf():

	noise_ratio_ls = linspace(1.5, 2.5, num=14)
	noise_period_ls = linspace(1.0, 1.0, num=1) # Content of this should be integers (1+max-min divisible by num)
	threeopt_threshold_ls = linspace(10.0, 10.0, num=1) # Same for this one
	backtrack_period_ls = linspace(1.0, 1.0, num=1) # Same for this one
	double_bridge_period_ls = linspace(10.0, 10.0, num=1) # Same for this one
	noise_iters_ratio_ls = linspace(0.8,1.0,num=10)

	#double_bridge_period_ls = append(double_bridge_period_ls,100000000)

	jobs = []
	for nr in noise_ratio_ls:
		for np in noise_period_ls:
			for tt in threeopt_threshold_ls:
				for bp in backtrack_period_ls:
					for dbp in double_bridge_period_ls:
						for nir in noise_iters_ratio_ls:
							jobs.append({
								"exe_path": exe_path,
								"noise_ratio": nr,
								"noise_period": np,
								"threeopt_threshold": tt,
								"backtrack_period": bp,
								"double_bridge_period": dbp,
								"noise_iters_ratio": nir,
								"input_filepaths": input_filepaths
							})

	jobs_count = len(jobs)
	job_duration = len(input_filepaths) * TSP_TIME_LIMIT
	total_duration = job_duration * jobs_count / concurrency

	print("Total jobs: {}".format(jobs_count))
	print("ETA: {} minute(s)".format(round(total_duration/60.0, 2)))
	
	manager = mp.Manager()
	results = manager.list()

	# No number of forks specified => use all available CPUs
	pool = mp.Pool(initializer=bf_init, initargs=(results,)) 
	pool.map(bf_run, jobs)

	# Sort by total_length
	results_by_length = sorted(results, key = lambda res : res["total_length"])

	# Sort by total_time
	results_by_time = sorted(results, key = lambda res : res["total_time"])

	best_by_length = results_by_length[0]
	best_by_time = results_by_time[0]

	best_len = best_by_length["total_length"]
	best_time = best_by_time["total_time"]

	shared_count = len(list(filter(lambda res : res["total_length"] == best_len, results_by_length)))-1

	print("\nBest by length: {}".format(best_by_length))
	print("Score shared with {} other combinations of parameters ({}%)".format(shared_count, round(100.0*shared_count/(jobs_count-1), 2)))
	print("\nBest by time: {}".format(best_by_time))

	if output_filepath is not None:
		results_to_csv(results_by_length, output_filepath)
		


parallel_bf()

