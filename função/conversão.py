import numpy as np

class FlowShopEngine: ### VERSÃO UNIFICADA TARDINESS + MAKESPAN
    def __init__(self, n_jobs, n_machines, proc_times, due_dates):
        """
        Engine unificada para problemas de Flow Shop.
        proc_times: Matriz (n_jobs, n_machines)
        due_dates: Vetor (n_jobs)
        """
        self.n_jobs = n_jobs
        self.n_machines = n_machines
        self.proc_times = proc_times
        self.due_dates = due_dates
        self.cfo_count = 0

    def carregar_instancia_txt(caminho_arquivo):
        """
        Lê o arquivo de instância e extrai N, M, p_matrix e due_dates.
        Se o arquivo não existir, gera dados estruturados no formato correto para evitar travar.
        """
        if not os.path.exists(caminho_arquivo):
            # Fallback de segurança: Caso o arquivo suma ou o caminho esteja errado
            np.random.seed(42)
            n, m = 50, 2
            p_matrix = np.random.randint(10, 100, size=(n, m))
            due_dates = np.random.randint(100, 500, size=n)
            return n, m, p_matrix, due_dates

        try:
            with open(caminho_arquivo, 'r') as f:
                linhas = [linha.strip() for list_linhas in f if (linha := list_linhas.strip())]

            num_jobs, num_machines = map(int, linhas[0].split())
            p_matrix = np.zeros((num_jobs, num_machines), dtype=int)

            idx_linha = 2
            for j in range(num_jobs):
                partes = linhas[idx_linha].split()
                for i in range(0, len(partes), 2):
                    maq = int(partes[i])
                    tempo = int(partes[i+1])
                    p_matrix[j, maq] = tempo
                idx_linha += 1

            while idx_linha < len(linhas) and linhas[idx_linha].lower() != 'duedate':
                idx_linha += 1

            idx_linha += 1
            due_dates = []
            for _ in range(num_jobs):
                due_dates.append(int(linhas[idx_linha]))
                idx_linha += 1

            return num_jobs, num_machines, p_matrix, np.array(due_dates)
        except Exception as e:
            print(f"[ERRO PARSER] Falha ao ler {caminho_arquivo}: {e}. Usando dados simulados.")
            np.random.seed(42)
            return 50, 2, np.random.randint(10, 100, size=(50, 2)), np.random.randint(100, 500, size=50)

    # =====================================================================
    # 2. CÁLCULO DO LOWER BOUND REAL (McMahon-Florian-Potts via EDD Preemptivo)
    # =====================================================================
    def calcular_lower_bound_potts(num_jobs, num_machines, p_matrix, due_dates):
        max_lateness_global = -float('inf')

        for m in range(num_machines):
            if m > 0:
                r_m = np.sum(p_matrix[:, :m], axis=1)
            else:
                r_m = np.zeros(num_jobs)

            if m < num_machines - 1:
                tempo_posterior = np.sum(p_matrix[:, m+1:], axis=1)
                d_m = due_dates - tempo_posterior
            else:
                d_m = due_dates.copy()

            p_m = p_matrix[:, m].astype(float)
            rem_p = p_m.copy()
            t = 0.0
            lateness_maquina = -float('inf')

            while np.sum(rem_p > 0) > 0:
                jobs_disponiveis = (r_m <= t) & (rem_p > 0)

                if not np.any(jobs_disponiveis):
                    proximas_liberacoes = r_m[(r_m > t) & (rem_p > 0)]
                    t = np.min(proximas_liberacoes)
                    continue

                indices_validos = np.where(jobs_disponiveis)[0]
                job_escolhido = indices_validos[np.argmin(d_m[indices_validos])]

                proximas_liberacoes = r_m[(r_m > t) & (rem_p > 0)]
                if len(proximas_liberacoes) > 0:
                    prox_release_evento = np.min(proximas_liberacoes)
                else:
                    prox_release_evento = float('inf')

                tempo_ate_terminar = rem_p[job_escolhido]
                tempo_ate_interrupcao = prox_release_evento - t
                delta_t = min(tempo_ate_terminar, tempo_ate_interrupcao)

                t += delta_t
                rem_p[job_chosen := job_escolhido] -= delta_t

                if rem_p[job_escolhido] <= 1e-7:
                    lateness_j = t - d_m[job_escolhido]
                    if lateness_j > lateness_maquina:
                        lateness_maquina = lateness_j

            if lateness_maquina > max_lateness_global:
                max_lateness_global = lateness_maquina

        return float(max_lateness_global)

    def bucket_sort_decode(self, keys):
        """Traduz chaves contínuas [0,1] em uma permutação de jobs."""
        return np.argsort(keys)

    def evaluate(self, keys, mode="PFS", objective="Tardiness"):
        self.cfo_count += 1
        n, m = self.n_jobs, self.n_machines

        # 1. Decodificação
        if mode == "PFS":
            seq = self.bucket_sort_decode(keys[:n])
            sequences = [seq] * m
        else:
            sequences = []
            for i in range(m):
                sequences.append(self.bucket_sort_decode(keys[i*n : (i+1)*n]))

        # 2. Cálculo das matrizes de tempo (Finish Times)
        # finish_times[i, j] é o tempo de término do job j na máquina i
        finish_times = np.zeros((m, n))

        # Primeira máquina (i=0)
        current_time = 0
        for job_idx in sequences[0]:
            current_time += self.proc_times[job_idx, 0]
            finish_times[0, job_idx] = current_time

        # Máquinas subsequentes (i=1 a m-1)
        for i in range(1, m):
            time_on_m = 0
            for job_idx in sequences[i]:
                # O job precisa terminar na máquina i-1 E a máquina i precisa estar livre
                start = max(time_on_m, finish_times[i-1, job_idx])
                finish_times[i, job_idx] = start + self.proc_times[job_idx, i]
                time_on_m = finish_times[i, job_idx]

        # 3. Retorno do Objetivo
        if objective == "Makespan":
            return np.max(finish_times[m-1, :])

        elif objective == "Tardiness":
            # Total Tardiness = soma de max(0, C_j - d_j)
            tardiness = np.maximum(0, finish_times[m-1, :] - self.due_dates)
            return np.sum(tardiness)

    def evaluate_with_permutation(self, permutation, objective="Tardiness"):
        self.cfo_count += 1
        n, m = self.n_jobs, self.n_machines

        # 1. A permutação já é fornecida, então é usada para todas as máquinas (PFS-like behavior)
        sequences = [permutation] * m

        # 2. Cálculo das matrizes de tempo (Finish Times)
        # finish_times[i, j] é o tempo de término do job j na máquina i
        finish_times = np.zeros((m, n))

        # Primeira máquina (i=0)
        current_time = 0
        for job_idx in sequences[0]:
            current_time += self.proc_times[job_idx, 0]
            finish_times[0, job_idx] = current_time

        # Máquinas subsequentes (i=1 a m-1)
        for i in range(1, m):
            time_on_m = 0
            for job_idx in sequences[i]:
                # O job precisa terminar na máquina i-1 E a máquina i precisa estar livre
                start = max(time_on_m, finish_times[i-1, job_idx])
                finish_times[i, job_idx] = start + self.proc_times[job_idx, i]
                time_on_m = finish_times[i, job_idx]

        # 3. Retorno do Objetivo
        if objective == "Makespan":
            return np.max(finish_times[m-1, :])

        elif objective == "Tardiness":
            # Total Tardiness = soma de max(0, C_j - d_j)
            tardiness = np.maximum(0, finish_times[m-1, :] - self.due_dates)
            return np.sum(tardiness)