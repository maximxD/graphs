import subprocess
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path
import re
import json
import os

def run_benchmark(executable, vertices, prob, delta, save=False):
    cmd = [f"./{executable}"]
    cmd.extend(["--vertices", str(vertices), "--prob", str(prob), "--delta", str(delta)])
    if save:
        cmd.extend(["--save"])
    else:
        cmd.extend(["graph.txt"])
    
    result = subprocess.run(cmd, capture_output=True, text=True, check=True)
    # Ищем все времена выполнения в выводе программы
    times = re.findall(r'реализация: (\d+\.\d+) секунд', result.stdout)
    if times:
        times = [float(t) for t in times]
        times.sort()
        times = times[2:-2]
        return np.mean(times)
    else:
        print(f"Не удалось найти время выполнения в выводе {executable}")
        print("Вывод программы:", result.stdout)
        return None

def save_intermediate_results(results, filename='results.json'):
    with open(filename, 'w') as f:
        json.dump(results, f)

def load_existing_results(filename='results.json'):
    if os.path.exists(filename):
        with open(filename, 'r') as f:
            return json.load(f)
    return {}

def calculate_speedup(cpp_times, parallel_times):
    """Вычисляет ускорение параллельной реализации относительно C++"""
    if len(cpp_times) != len(parallel_times):
        return None
    speedups = []
    for i in range(len(cpp_times)):
        if parallel_times[i] > 0:
            speedup = cpp_times[i] / parallel_times[i]
            speedups.append(speedup)
        else:
            speedups.append(0)
    return speedups

def main():
    # Список исполняемых файлов
    # executables = ['main-cpp.o', 'main-dpc-cpu.o', 'main-dpc-gpu.o', 'main-openmp-cpu.o']
    executables = ['main-cpp.o', 'main-dpc-cpu.o', 'main-dpc-gpu.o', 'main-openmp-cpu.o', 'main-openmp-gpu.o']
    # labels = ['C++', 'DPC++ CPU', 'DPC++ GPU', 'OpenMP CPU']
    labels = ['C++', 'DPC++ CPU', 'DPC++ CPU+GPU', 'OpenMP CPU', 'OpenMP CPU+GPU']
    
    # Создаем директории для результатов
    Path('benchmarks').mkdir(exist_ok=True)

    # Загружаем существующие результаты
    results = load_existing_results()

    limit_by_prob = {
        0.1: 25000,
        0.2: 25000,
        0.3: 25000,
        0.4: 25000,
        0.5: 25000,
        0.6: 20000,
        0.7: 20000,
        0.8: 20000,
        0.9: 20000
    }

    probs = [0.9]
    # probs = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9]
    deltas = [1, 60, 100]
    # deltas = [11, 21, 31, 41, 51, 61, 71, 81, 91, 101]
    vertices_by_prob = {}
    for prob in probs:
        vertices_by_prob[prob] = []
        vertices = 1000
        while vertices <= limit_by_prob[prob]:
        # while vertices**2 * prob <= 150_000_000:
            vertices_by_prob[prob].append(vertices)
            if vertices < 10000:
                vertices += 1000
            elif vertices < 20000:
                vertices += 2500
            else:
                vertices += 5000
    
    # Для каждой вероятности ребра и каждого значения delta создаем отдельный график
    for prob in probs:
        for delta in deltas:
            result_key = f"prob_{prob}_delta_{delta}"
            
            # Проверяем, есть ли уже результаты для этой комбинации
            results_completed = False
            if result_key in results:
                results_completed = True
                print(f"\nИспользуем существующие результаты для вероятности {prob} и delta {delta}")
            else:
                print(f"\nТестирование для вероятности ребра {prob} и delta {delta}")
            
            # Создаем график времени выполнения
            plt.figure(figsize=(12, 8))

            avgs = [[] for _ in range(len(executables))]
            vertices = vertices_by_prob[prob]
        
            # Запускаем бенчмарки для каждого количества вершин
            if not results_completed:
                for v in vertices:
                    # Для каждой реализации
                    for i, exe in enumerate(executables):
                        print(f"Тестирование {exe} для {v} вершин")
                        
                        if i == 0:  # Для первой реализации сохраняем граф
                            avg = run_benchmark(exe, v, prob, delta, save=True)
                        else:
                            avg = run_benchmark(exe, v, prob, delta)
                        
                        if avg is not None:
                            avgs[i].append(avg)
                            print(f"Среднее время = {avg:.6f}")

                # Сохраняем промежуточные результаты после каждого теста
                results[result_key] = {
                    "avgs": avgs,
                    "vertices": vertices[:len(avgs[0])]
                }
                save_intermediate_results(results)
            else:
                for i in range(len(executables)):
                    avgs[i] = results[result_key]["avgs"][i]
                    vertices = results[result_key]["vertices"]

                # avgs = results[result_key]["avgs"][:len(executables)]
                # vertices = results[result_key]["vertices"][:len(executables)]

            # Строим график времени выполнения для текущей реализации
            for i, exe in enumerate(executables):
                plt.plot(vertices, avgs[i], label=labels[i], marker='o', linewidth=2)
            
            # Настраиваем график времени выполнения
            plt.xlabel('Количество вершин')
            plt.ylabel('Время выполнения (сек)')
            plt.title(f'Зависимость времени выполнения от количества вершин\n(вероятность ребра = {prob}, delta = {delta})')
            plt.grid(True, linestyle='--', alpha=0.7)
            plt.legend()
            
            # Сохраняем график времени выполнения
            plt.savefig(f'benchmarks/vertices_vs_time_prob_{prob}_delta_{delta}.png', dpi=300, bbox_inches='tight')
            plt.close()

            # Создаем график ускорения
            plt.figure(figsize=(12, 8))
            
            # Вычисляем ускорение для каждой параллельной реализации относительно C++
            cpp_times = avgs[0]  # Время C++ реализации
            speedup_labels = ['DPC++ CPU', 'DPC++ CPU+GPU', 'OpenMP CPU', 'OpenMP CPU+GPU']
            
            for i in range(1, len(executables)):
                parallel_times = avgs[i]
                speedups = calculate_speedup(cpp_times, parallel_times)
                if speedups:
                    plt.plot(vertices, speedups, label=speedup_labels[i-1], marker='o', linewidth=2)
            
            plt.axhline(y=1, color='black', linestyle='--', alpha=0.5, label='C++')
            
            # Настраиваем график ускорения
            plt.xlabel('Количество вершин')
            plt.ylabel('Ускорение')
            plt.title(f'Ускорение параллельных реализаций относительно последовательной\n(вероятность ребра = {prob}, delta = {delta})')
            plt.grid(True, linestyle='--', alpha=0.7)
            plt.legend()
            
            # Сохраняем график ускорения
            plt.savefig(f'benchmarks/speedup_vs_vertices_prob_{prob}_delta_{delta}.png', dpi=300, bbox_inches='tight')
            plt.close()

if __name__ == '__main__':
    main()
