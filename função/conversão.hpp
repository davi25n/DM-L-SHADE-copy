#ifndef CONVERSAO_HPP
#define CONVERSAO_HPP

#include <vector>
#include <string>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <limits>
#include <random>

namespace FlowShop {

    struct InstanceData {
        int n_jobs;
        int n_machines;
        std::vector<std::vector<int>> proc_times; // Matriz (n_jobs, n_machines)
        std::vector<int> due_dates;                // Vetor (n_jobs)
    };

    class FlowShopEngine {
    private:
        int n_jobs;
        int n_machines;
        std::vector<std::vector<int>> proc_times;
        std::vector<int> due_dates;
        unsigned long long cfo_count;

    public:
        FlowShopEngine(int nj, int nm, const std::vector<std::vector<int>>& pt, const std::vector<int>& dd)
            : n_jobs(nj), n_machines(nm), proc_times(pt), due_dates(dd), cfo_count(0) {}

        // Resetador e contadores
        unsigned long long getCfoCount() const { return cfo_count; }
        void resetCfoCount() { cfo_count = 0; }

        // Mapeamento de métodos estáticos
        static InstanceData carregar_instancia_txt(const std::string& caminho_arquivo);
        static double calcular_lower_bound_potts(int num_jobs, int num_machines, 
                                                 const std::vector<std::vector<int>>& p_matrix, 
                                                 const std::vector<int>& due_dates);

        // Métodos de Decodificação e Avaliação
        std::vector<int> bucket_sort_decode(const std::vector<double>& keys, int start_idx, int length) const;
        
        double evaluate(const std::vector<double>& keys, 
                         const std::string& mode = "PFS", 
                         const std::string& objective = "Tardiness");

        double evaluate_with_permutation(const std::vector<int>& permutation, 
                                         const std::string& objective = "Tardiness");
    };

} // namespace FlowShop

#endif // CONVERSAO_HPP