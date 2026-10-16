/*
  L-SHADE implemented by C++ for Special Session & Competition on Real-Parameter Single Objective Optimization at CEC-2014
  See the details of L-SHADE in the following paper:

  * Ryoji Tanabe and Alex Fukunaga: Improving the Search Performance of SHADE Using Linear Population Size Reduction,  Proc. IEEE Congress on Evolutionary Computation (CEC-2014), Beijing, July, 2014.
  
  Version: 1.1  Date: 9/Jun/2014
  Written by Ryoji Tanabe (rt.ryoji.tanabe [at] gmail.com)
*/
#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>
#include "de.h"
#include "conversão.hpp"

#include <stdexcept>
#include <chrono>

double *OShift,*M,*y,*z,*x_bound;
int ini_flag=0,n_flag,func_flag,*SS;

int g_function_number;
int g_problem_size;
unsigned int g_max_num_evaluations;

const int MAX_JOBS = 500;
unsigned int n_jobs;
unsigned int n_machines;
unsigned int jobs[MAX_JOBS];// Array to store the maximum time for each job


int g_pop_size;
double g_arc_rate;
int g_memory_size;
double g_p_best_rate;
FlowShop::FlowShopEngine *g_flowshop_engine = nullptr;

void cleanup_binary_func();

// prints a duration in seconds with fixed notation, keeping cout's previous format
static void print_seconds(double seconds) {
  std::ios_base::fmtflags old_flags = std::cout.flags();
  std::streamsize old_precision = std::cout.precision();
  std::cout << std::fixed << std::setprecision(4) << seconds << " s";
  std::cout.flags(old_flags);
  std::cout.precision(old_precision);
}

int main(int argc, char **argv) {
  string instance_path;
  //number of runs
  int num_runs = 10;
    //dimension size. please select from 10, 30, 50, 100
  g_problem_size = 10;
  //available number of fitness evaluations 
  g_max_num_evaluations = g_problem_size * 10000;

  //random seed is selected based on time according to competition rules
  srand((unsigned)time(NULL));

  //L-SHADE parameters
  g_pop_size = (int)round(g_problem_size * 18);
  g_memory_size = 6;
  g_arc_rate = 2.6;
  g_p_best_rate = 0.11;

  // DM-L-SHADE parameters
  double elite_rate = 0.1;
  double clusters_rate = 0.1468;
  int mining_generation_step = 168;

  // Parse command-line arguments for function number
  int function_start = 1;
  int function_end = 31;
  
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--f") == 0 && i + 1 < argc) {
      int func_num = atoi(argv[i + 1]);
      if (func_num >= 1 && func_num <= 31) {
        function_start = func_num;
        function_end = func_num;
      } else {
        cerr << "Invalid function number. Please use a value between 1 and 31." << endl;
        return 1;
      }
      break;
    }
    else if (strcmp(argv[i], "--i") == 0 && i + 1 < argc) {
      instance_path = "src/instancias/" + string(argv[i + 1]);
      function_start = 1;
      function_end = 1;
    }
  }
  if (instance_path.empty() && function_start != function_end) {
    cerr << "Error: No input file specified. Please use the --i option to provide an input file." << endl;
    return 1;
  }

  FlowShop::InstanceData instance;
  if (!instance_path.empty()) {
    try {
      instance = FlowShop::FlowShopEngine::carregar_instancia_txt(instance_path);
      g_problem_size = instance.n_jobs;
      g_max_num_evaluations = (g_problem_size * 10000);
      g_pop_size = (int)round(g_problem_size * 18);
      g_flowshop_engine = new FlowShop::FlowShopEngine(instance.n_jobs, instance.n_machines, instance.proc_times, instance.due_dates);
    } catch (const std::exception &error) {
      cerr << "Error loading instance: " << error.what() << endl;
      return 1;
    }
  }
  //int i = function_start - 1;
  if (function_start == function_end) {
    g_function_number = function_start;
    cout << "\n-------------------------------------------------------" << endl;
    cout << "Function = " << g_function_number << ", Dimension size = " << g_problem_size << "\n" << endl;

    Fitness *bsf_fitness_array = (Fitness*)malloc(sizeof(Fitness) * num_runs);
    Fitness mean_bsf_fitness = 0;
    Fitness std_bsf_fitness = 0; 
    double *run_time_array = (double*)malloc(sizeof(double) * num_runs);
    auto total_start = std::chrono::steady_clock::now();

    for (int j = 0; j < num_runs; j++) { 
      //searchAlgorithm *alg = new LSHADE();
      int max_elite_size = std::round(elite_rate * g_pop_size);
      int number_of_patterns = std::round(clusters_rate * max_elite_size);
      searchAlgorithm *alg = new DMLSHADE(max_elite_size, number_of_patterns, mining_generation_step);
      auto run_start = std::chrono::steady_clock::now();
      bsf_fitness_array[j] = alg->run();
      auto run_end = std::chrono::steady_clock::now();
      run_time_array[j] = std::chrono::duration<double>(run_end - run_start).count();
      cout << j + 1 << "th run, " << "error value = " << bsf_fitness_array[j] << ", time = ";
      print_seconds(run_time_array[j]);
      cout << endl;
    }
    for (int j = 0; j < num_runs; j++) mean_bsf_fitness += bsf_fitness_array[j];
    mean_bsf_fitness /= num_runs;
    for (int j = 0; j < num_runs; j++) std_bsf_fitness += pow((mean_bsf_fitness - bsf_fitness_array[j]), 2.0);
    std_bsf_fitness = sqrt(std_bsf_fitness / num_runs);

    double total_time = std::chrono::duration<double>(std::chrono::steady_clock::now() - total_start).count();
    double mean_time = 0, std_time = 0;
    for (int j = 0; j < num_runs; j++) mean_time += run_time_array[j];
    mean_time /= num_runs;
    for (int j = 0; j < num_runs; j++) std_time += pow((mean_time - run_time_array[j]), 2.0);
    std_time = sqrt(std_time / num_runs);

    cout  << "\nmean = " << mean_bsf_fitness << ", std = " << std_bsf_fitness << endl;
    cout  << "time: mean = "; print_seconds(mean_time);
    cout  << ", std = "; print_seconds(std_time);
    cout  << ", total = "; print_seconds(total_time);
    cout  << endl;
    free(bsf_fitness_array);
    free(run_time_array);
  }
  delete g_flowshop_engine;
  cleanup_binary_func();
  return 0;
}
