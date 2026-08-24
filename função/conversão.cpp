#include "conversão.hpp"

namespace FlowShop {

    // =====================================================================
    // 1. CARREGAMENTO DA INSTÂNCIA DE ARQUIVO TXT
    // =====================================================================
    InstanceData FlowShopEngine::carregar_instancia_txt(const std::string& caminho_arquivo) {
        InstanceData inst;
        std::ifstream file(caminho_arquivo);

        // Fallback de segurança se o arquivo não existir
        if (!file.is_open()) {
            std::cout << "[ERRO PARSER] Falha ao abrir " << caminho_arquivo << ". Gerando dados simulados.\n";
            inst.n_jobs = 50;
            inst.n_machines = 2;
            inst.proc_times.assign(50, std::vector<int>(2, 0));
            inst.due_dates.assign(50, 0);

            std::mt19937 gen(42);
            std::uniform_int_distribution<int> dist_p(10, 99);
            std::uniform_int_distribution<int> dist_d(100, 499);

            for (int j = 0; j < 50; ++j) {
                inst.due_dates[j] = dist_d(gen);
                for (int m = 0; m < 2; ++m) {
                    inst.proc_times[j][m] = dist_p(gen);
                }
            }
            return inst;
        }

        try {
            std::string line;
            // Lê N e M
            while (std::getline(file, line) && line.empty());
            std::stringstream ss(line);
            ss >> inst.n_jobs >> inst.n_machines;

            inst.proc_times.assign(inst.n_jobs, std::vector<int>(inst.n_machines, 0));

            // Lê tempos de processamento
            for (int j = 0; j < inst.n_jobs; ++j) {
                while (std::getline(file, line) && line.empty());
                std::stringstream line_ss(line);
                int maq, tempo;
                while (line_ss >> maq >> tempo) {
                    if (maq >= 0 && maq < inst.n_machines) {
                        inst.proc_times[j][maq] = tempo;
                    }
                }
            }

            // Procura pela tag 'duedate'
            while (std::getline(file, line)) {
                std::string token = line;
                // Remove espaços em branco
                token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());
                std::transform(token.begin(), token.end(), token.begin(), ::tolower);
                if (token.find("duedate") != std::string::npos) break;
            }

            // Lê as datas de vencimento
            inst.due_dates.reserve(inst.n_jobs);
            for (int j = 0; j < inst.n_jobs; ++j) {
                int dd;
                file >> dd;
                inst.due_dates.push_back(dd);
            }

            return inst;
        } catch (...) {
            std::cout << "[ERRO PARSER] Exceção ao ler " << caminho_arquivo << ". Gerando simulados.\n";
            inst.n_jobs = 50;
            inst.n_machines = 2;
            inst.proc_times.assign(50, std::vector<int>(2, 50));
            inst.due_dates.assign(50, 200);
            return inst;
        }
    }

    // =====================================================================
    // 2. LOWER BOUND POTTS (EDD Preemptivo)
    // =====================================================================
    double FlowShopEngine::calcular_lower_bound_potts(int num_jobs, int num_machines, 
                                                       const std::vector<std::vector<int>>& p_matrix, 
                                                       const std::vector<int>& due_dates) {
        double max_lateness_global = -std::numeric_limits<double>::infinity();

        for (int m = 0; m < num_machines; ++m) {
            std::vector<double> r_m(num_jobs, 0.0);
            std::vector<double> d_m(num_jobs, 0.0);
            std::vector<double> rem_p(num_jobs, 0.0);

            for (int j = 0; j < num_jobs; ++j) {
                // Cálculo de release times (r_m)
                if (m > 0) {
                    for (int k = 0; k < m; ++k) r_m[j] += p_matrix[j][k];
                }
                // Cálculo de modified due dates (d_m)
                double tempo_posterior = 0.0;
                if (m < num_machines - 1) {
                    for (int k = m + 1; k < num_machines; ++k) tempo_posterior += p_matrix[j][k];
                }
                d_m[j] = static_cast<double>(due_dates[j]) - tempo_posterior;
                rem_p[j] = static_cast<double>(p_matrix[j][m]);
            }

            double t = 0.0;
            double lateness_maquina = -std::numeric_limits<double>::infinity();

            auto tem_trabalho_pendente = [&]() {
                for (double p : rem_p) if (p > 1e-7) return true;
                return false;
            };

            while (tem_trabalho_pendente()) {
                std::vector<int> indices_validos;
                for (int j = 0; j < num_jobs; ++j) {
                    if (r_m[j] <= t && rem_p[j] > 1e-7) {
                        indices_validos.push_back(j);
                    }
                }

                if (indices_validos.empty()) {
                    double prox_t = std::numeric_limits<double>::infinity();
                    for (int j = 0; j < num_jobs; ++j) {
                        if (r_m[j] > t && rem_p[j] > 1e-7) {
                            prox_t = std::min(prox_t, r_m[j]);
                        }
                    }
                    t = prox_t;
                    continue;
                }

                int job_escolhido = indices_validos[0];
                double min_due = d_m[job_escolhido];
                for (int idx : indices_validos) {
                    if (d_m[idx] < min_due) {
                        min_due = d_m[idx];
                        job_escolhido = idx;
                    }
                }

                double prox_release_evento = std::numeric_limits<double>::infinity();
                for (int j = 0; j < num_jobs; ++j) {
                    if (r_m[j] > t && rem_p[j] > 1e-7) {
                        prox_release_evento = std::min(prox_release_evento, r_m[j]);
                    }
                }

                double tempo_ate_terminar = rem_p[job_escolhido];
                double tempo_ate_interrupcao = prox_release_evento - t;
                double delta_t = std::min(tempo_ate_terminar, tempo_ate_interrupcao);

                t += delta_t;
                rem_p[job_escolhido] -= delta_t;

                if (rem_p[job_escolhido] <= 1e-7) {
                    double lateness_j = t - d_m[job_escolhido];
                    if (lateness_j > lateness_maquina) {
                        lateness_maquina = lateness_j;
                    }
                }
            }

            if (lateness_maquina > max_lateness_global) {
                max_lateness_global = lateness_maquina;
            }
        }

        return max_lateness_global;
    }

    // =====================================================================
    // 3. BUCKET SORT / ARGSORT DECODING
    // =====================================================================
    std::vector<int> FlowShopEngine::bucket_sort_decode(const std::vector<double>& keys, int start_idx, int length) const {
        std::vector<int> indices(length);
        std::iota(indices.begin(), indices.end(), 0);

        std::sort(indices.begin(), indices.end(), [&](int a, int b) {
            return keys[start_idx + a] < keys[start_idx + b];
        });

        return indices;
    }

    // =====================================================================
    // 4. AVALIAÇÃO COM CHAVES CONTINUAS (EVALUATE)
    // =====================================================================
    double FlowShopEngine::evaluate(const std::vector<double>& keys, const std::string& mode, const std::string& objective) {
        cfo_count++;
        int n = n_jobs;
        int m = n_machines;

        std::vector<std::vector<int>> sequences;

        if (mode == "PFS") {
            std::vector<int> seq = bucket_sort_decode(keys, 0, n);
            sequences.assign(m, seq);
        } else {
            for (int i = 0; i < m; ++i) {
                sequences.push_back(bucket_sort_decode(keys, i * n, n));
            }
        }

        std::vector<std::vector<double>> finish_times(m, std::vector<double>(n, 0.0));

        // Primeira máquina
        double current_time = 0;
        for (int job_idx : sequences[0]) {
            current_time += proc_times[job_idx][0];
            finish_times[0][job_idx] = current_time;
        }

        // Máquinas subsequentes
        for (int i = 1; i < m; ++i) {
            double time_on_m = 0;
            for (int job_idx : sequences[i]) {
                double start = std::max(time_on_m, finish_times[i - 1][job_idx]);
                finish_times[i][job_idx] = start + proc_times[job_idx][i];
                time_on_m = finish_times[i][job_idx];
            }
        }

        if (objective == "Makespan") {
            double max_makespan = 0.0;
            for (int j = 0; j < n; ++j) {
                max_makespan = std::max(max_makespan, finish_times[m - 1][j]);
            }
            return max_makespan;
        } else if (objective == "Tardiness") {
            double total_tardiness = 0.0;
            for (int j = 0; j < n; ++j) {
                double tard = finish_times[m - 1][j] - static_cast<double>(due_dates[j]);
                if (tard > 0) total_tardiness += tard;
            }
            return total_tardiness;
        }

        return 0.0;
    }

    // =====================================================================
    // 5. AVALIAÇÃO COM PERMUTAÇÃO DIRETA
    // =====================================================================
    double FlowShopEngine::evaluate_with_permutation(const std::vector<int>& permutation, const std::string& objective) {
        cfo_count++;
        int n = n_jobs;
        int m = n_machines;

        std::vector<std::vector<double>> finish_times(m, std::vector<double>(n, 0.0));

        // Primeira máquina
        double current_time = 0;
        for (int job_idx : permutation) {
            current_time += proc_times[job_idx][0];
            finish_times[0][job_idx] = current_time;
        }

        // Máquinas subsequentes
        for (int i = 1; i < m; ++i) {
            double time_on_m = 0;
            for (int job_idx : permutation) {
                double start = std::max(time_on_m, finish_times[i - 1][job_idx]);
                finish_times[i][job_idx] = start + proc_times[job_idx][i];
                time_on_m = finish_times[i][job_idx];
            }
        }

        if (objective == "Makespan") {
            double max_makespan = 0.0;
            for (int j = 0; j < n; ++j) {
                max_makespan = std::max(max_makespan, finish_times[m - 1][j]);
            }
            return max_makespan;
        } else if (objective == "Tardiness") {
            double total_tardiness = 0.0;
            for (int j = 0; j < n; ++j) {
                double tard = finish_times[m - 1][j] - static_cast<double>(due_dates[j]);
                if (tard > 0) total_tardiness += tard;
            }
            return total_tardiness;
        }

        return 0.0;
    }

} // namespace FlowShop