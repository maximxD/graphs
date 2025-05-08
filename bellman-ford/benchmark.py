import subprocess
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path
import re
import json

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
        return np.mean(times), np.std(times)
    else:
        print(f"Не удалось найти время выполнения в выводе {executable}")
        print("Вывод программы:", result.stdout)
        return None, None

def main():
    # Список исполняемых файлов
    results = {}
    executables = ['main-cpp.o', 'main-dpc.o', 'main-openmp.o']
    labels = ['C++', 'DPC++', 'OpenMP']
    
    # Создаем директории для результатов
    Path('benchmarks').mkdir(exist_ok=True)

    # vertices = [100, 500, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000, 12500, 15000]
    probs = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9]
    vertices_by_prob = {}
    for prob in probs:
        vertices_by_prob[prob] = []
        vertices = 100
        while vertices**2 * prob < 300_000_000:
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
        print(f"\nТестирование для вероятности ребра {prob}")
        
        # Создаем график
        plt.figure(figsize=(12, 8))

        means = [[], [], []]
        stds = [[], [], []]
        vertices = vertices_by_prob[prob]
    
        # Запускаем бенчмарки для каждого количества вершин
        for v in vertices:
            # Для каждой реализации
            for i, exe in enumerate(executables):
                print(f"Тестирование {exe} для {v} вершин")
                
                if i == 0:  # Для первой реализации сохраняем граф
                    mean, std = run_benchmark(exe, v, prob, save=True)
                else:
                    mean, std = run_benchmark(exe, v, prob)
                
                if mean is not None:
                    means[i].append(mean)
                    stds[i].append(std)
                    print(f"Среднее время = {mean:.6f} ± {std:.6f} сек")

        results[prob] = {
            "means": means,
            "stds": stds,
            "vertices": vertices
        }

        # Строим график для текущей реализации
        for i, exe in enumerate(executables):
            plt.errorbar(vertices, means[i], yerr=stds[i], label=labels[i], 
                        marker='o', capsize=5, linewidth=2)
        
        # Настраиваем график
        plt.xlabel('Количество вершин')
        plt.ylabel('Время выполнения (сек)')
        plt.title(f'Зависимость времени выполнения от количества вершин\n(вероятность ребра = {prob})')
        plt.grid(True, linestyle='--', alpha=0.7)
        plt.legend()
        
        # Сохраняем график
        plt.savefig(f'benchmarks/vertices_vs_time_prob_{prob}.png', dpi=300, bbox_inches='tight')
        plt.close()
    
    with open('results.json', 'w') as f:
        json.dump(results, f)

if __name__ == '__main__':
    main()
