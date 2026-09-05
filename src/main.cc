/*
  L-SHADE implemented by C++ for Special Session & Competition on Real-Parameter Single Objective Optimization at CEC-2014
  See the details of L-SHADE in the following paper:

  * Ryoji Tanabe and Alex Fukunaga: Improving the Search Performance of SHADE Using Linear Population Size Reduction,  Proc. IEEE Congress on Evolutionary Computation (CEC-2014), Beijing, July, 2014.
  
  Version: 1.1  Date: 9/Jun/2014
  Written by Ryoji Tanabe (rt.ryoji.tanabe [at] gmail.com)
*/
#include <cstdio>
#include <fstream>
#include "de.h"

double *OShift,*M,*y,*z,*x_bound;
int ini_flag=0,n_flag,func_flag,*SS;

int g_function_number;
int g_problem_size;
unsigned int g_max_num_evaluations;

unsigned int n_jobs;
unsigned int n_machines;
unsigned int jobs;
unsigned int machines;

int g_pop_size;
double g_arc_rate;
int g_memory_size;
double g_p_best_rate;

void cleanup_binary_func();

int main(int argc, char **argv) {
  std::ifstream file;
  //number of runs
  int num_runs = 51;
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
      file.open(argv[i + 1]);
      if (!file.is_open()) {
        cerr << "Error: Cannot open input file for reading." << endl;
        return 1;
      }
    }
  }
  if (!file.is_open() && (function_start != function_end)) {
    cerr << "Error: No input file specified. Please use the --i option to provide an input file." << endl;
    return 1;
  }

  if (function_start==function_end) {//condicional para rodar todas as funcoes do CEC2014
    for (int i = function_start - 1; i < function_end; i++) {
      g_function_number = i + 1;
      cout << "\n-------------------------------------------------------" << endl;
      cout << "Function = " << g_function_number << ", Dimension size = " << g_problem_size << "\n" << endl;

      Fitness *bsf_fitness_array = (Fitness*)malloc(sizeof(Fitness) * num_runs);
      Fitness mean_bsf_fitness = 0;
      Fitness std_bsf_fitness = 0;

      for (int j = 0; j < num_runs; j++) { 
        //searchAlgorithm *alg = new LSHADE();
        int max_elite_size = std::round(elite_rate * g_pop_size);
        int number_of_patterns = std::round(clusters_rate * max_elite_size);
        searchAlgorithm *alg = new DMLSHADE(max_elite_size, number_of_patterns, mining_generation_step);
        bsf_fitness_array[j] = alg->run();
        cout << j + 1 << "th run, " << "error value = " << bsf_fitness_array[j] << endl;
      }
    
      for (int j = 0; j < num_runs; j++) mean_bsf_fitness += bsf_fitness_array[j];
      mean_bsf_fitness /= num_runs;

      for (int j = 0; j < num_runs; j++) std_bsf_fitness += pow((mean_bsf_fitness - bsf_fitness_array[j]), 2.0);
      std_bsf_fitness /= num_runs;
      std_bsf_fitness = sqrt(std_bsf_fitness);

      cout  << "\nmean = " << mean_bsf_fitness << ", std = " << std_bsf_fitness << endl;
      free(bsf_fitness_array);
    }
  } 
  else {//condicional para rodar as instancias polinomiais

    char buffer[1024];
    
    //le a primeira linha do arquivo e divide jobs e machines
    if (!file.getline(buffer, sizeof(buffer)) || sscanf(buffer, "%u %u", &n_jobs, &n_machines) != 2) {
      cerr << "Error: Invalid first line. Expected: jobs machines." << endl;
      return 1;//testa se a primeira linha do arquivo tem jobs e machines
    }
    cout << "jobs = " << n_jobs << ", machines = " << n_machines << endl;

    // Read the second line (header) and ignore it
    file.getline(buffer, sizeof(buffer)); 

    // Read every line of jobs and store in vector jobs
    for (int i = 0; i < n_jobs; ++i) {
      if (!file.getline(buffer, sizeof(buffer))) {
        cerr << "Error: Not enough lines in the input file for job data." << endl;
        return 1;//testa se o arquivo tem linhas suficientes para os jobs
      }
      std::string linha(buffer);
      cout << linha << endl;
    }
    while (file.getline(buffer, sizeof(buffer))) {
      std::string linha(buffer);
      cout << linha << endl;
    }

    //  incluir chamada das funções polinomiais do npfs_test_func()
  }
  cleanup_binary_func();
  return 0;
}
