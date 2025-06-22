import subprocess
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path
import re
import json
import os

def run_benchmark(executable, vertices, prob, save=False):
    cmd = [f"./{executable}"]
    cmd.extend(["--vertices", str(vertices), "--prob", str(prob)])
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
    executables = ['main-cpp.o', 'main-dpc-cpu.o', 'main-dpc-gpu.o', 'main-openmp-cpu.o', 'main-openmp-gpu.o']
    labels = ['C++', 'DPC++ CPU', 'DPC++ CPU+GPU', 'OpenMP CPU', 'OpenMP CPU+GPU']
    
    # Создаем директории для результатов
    Path('benchmarks').mkdir(exist_ok=True)

    # Загружаем существующие результаты
    results = load_existing_results()

    probs = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9]
    vertices_by_prob = {}
    for prob in probs:
        vertices_by_prob[prob] = []
        vertices = 100
        while vertices**2 * prob <= 250_000_000:
            vertices_by_prob[prob].append(vertices)
            if vertices < 500:
                vertices += 100
            elif vertices < 1000:
                vertices += 500
            elif vertices < 10000:
                vertices += 1000
            elif vertices < 20000:
                vertices += 2500
            else:
                vertices += 5000
    
    # Для каждой вероятности ребра создаем отдельный график
    for prob in probs:
        # Проверяем, есть ли уже результаты для этой вероятности
        results_completed = False
        if str(prob) in results:
            results_completed = True
            print(f"\nИспользуем существующие результаты для вероятности {prob}")
        else:
            print(f"\nТестирование для вероятности ребра {prob}")
        
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
                        avg = run_benchmark(exe, v, prob, save=True)
                    else:
                        avg = run_benchmark(exe, v, prob)
                    
                    if avg is not None:
                        avgs[i].append(avg)
                        print(f"Среднее время = {avg:.6f}")

                # Сохраняем промежуточные результаты после каждого теста
                results[str(prob)] = {
                    "avgs": avgs,
                    "vertices": vertices[:len(avgs[0])]
                }
                save_intermediate_results(results)
        else:
            # Используем существующие результаты
            for i in range(len(executables)):
                avgs[i] = results[str(prob)]["avgs"][i]
            vertices = results[str(prob)]["vertices"]

        # Строим график времени выполнения для текущей реализации
        for i, exe in enumerate(executables):
            plt.plot(vertices, avgs[i], label=labels[i], marker='o', linewidth=2)
        
        # Настраиваем график времени выполнения
        plt.xlabel('Количество вершин')
        plt.ylabel('Время выполнения (сек)')
        plt.title(f'Зависимость времени выполнения от количества вершин\n(вероятность ребра = {prob})')
        plt.grid(True, linestyle='--', alpha=0.7)
        plt.legend()
        
        # Сохраняем график времени выполнения
        plt.savefig(f'benchmarks/vertices_vs_time_prob_{prob}.png', dpi=300, bbox_inches='tight')
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
        plt.title(f'Ускорение параллельных реализаций относительно последовательной\n(вероятность ребра = {prob})')
        plt.grid(True, linestyle='--', alpha=0.7)
        plt.legend()
        
        # Сохраняем график ускорения
        plt.savefig(f'benchmarks/speedup_vs_vertices_prob_{prob}.png', dpi=300, bbox_inches='tight')
        plt.close()

if __name__ == '__main__':
    main()
