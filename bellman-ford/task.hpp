#pragma once

#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <set>
#include <iomanip>
#include "../common/graph.hpp"

typedef std::vector<int> (*BellmanFordImpl)(int vertices, std::vector<Edge> edges, int source, std::chrono::duration<double>& duration);

struct Impl {
    std::vector<int> (*bellman_ford_impl)(int vertices, std::vector<Edge> edges, int source, std::chrono::duration<double>& duration);
    std::string impl_name;
};

class Task {
    bool should_print_results = false;
public:
    Task(std::vector<Impl> impls) : impls(impls) {}

    void run() {
        std::vector<Edge> edges = graph.get_edges();
        std::vector<std::vector<int>> dists(impls.size());
        int source = 0;

        std::chrono::_V2::system_clock::time_point start;
        std::chrono::_V2::system_clock::time_point stop;
        std::vector<int> reference_dist;

        for (int i = 0; i < impls.size(); i++) {
            std::chrono::duration<double> duration;
            dists[i] = impls[i].bellman_ford_impl(vertices, edges, source, duration);
            std::cout << std::setw(15) << std::left << impls[i].impl_name << "реализация: " << std::fixed << std::setprecision(6) << duration.count() << " секунд" << std::endl;
            if (reference_dist.empty()) reference_dist = dists[i];
        }

        if (impls.size() > 1) { 
            for (int i = 0; i < impls.size(); i++) {
                if (should_print_results) {
                    std::cout << impls[i].impl_name << " реализация: ";
                    for (int j = 0; j < vertices; j++) {
                        if (dists[i][j] == INF) std::cout << "INF ";
                        else std::cout << dists[i][j] << " ";
                    }
                    std::cout << std::endl;
                }
                if (dists[i] != reference_dist) {
                    std::cout << "Результаты не совпадают" << std::endl;
                    return;
                }
            }
            std::cout << "Результаты совпадают" << std::endl;
        }
    }

    int init(int argc, char* argv[]) {
        bool should_save_graph = false;
        std::string graph_file;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--vertices" && i + 1 < argc) {
                vertices = std::atoi(argv[++i]);
                if (vertices <= 0) {
                    std::cerr << "Ошибка: количество вершин должно быть положительным числом" << std::endl;
                    return 1;
                }
            } else if (arg == "--prob" && i + 1 < argc) {
                edge_probability = std::atof(argv[++i]);
                if (edge_probability <= 0 || edge_probability > 1) {
                    std::cerr << "Ошибка: вероятность ребра должна быть в диапазоне (0, 1]" << std::endl;
                    return 1;
                }
            } else if (arg == "--save") {
                should_save_graph = true;
            } else if (arg == "--print") {
                should_print_results = true;
            } else if (arg == "--help") {
                print_usage(argv[0]);
                return 0;
            } else if (arg[0] != '-') {
                graph_file = arg;
            } else {
                std::cerr << "Неизвестная опция: " << arg << std::endl;
                print_usage(argv[0]);
                return 1;
            }
        }

        if (!graph_file.empty()) {
            try {
                graph.load_from_file(graph_file);
                std::cout << "Граф загружен из файла: " << graph_file << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Ошибка при загрузке графа: " << e.what() << std::endl;
                return 1;
            }
        } else {
            std::cout << "Создание случайного графа, количество вершин: " << vertices << ", вероятность ребра: " << edge_probability << std::endl;
            graph.create_random_graph(vertices, edge_probability);
            if (should_save_graph) {
                graph_file = "graph.txt";
                graph.save_to_file(graph_file);
                std::cout << "Граф сохранен в файл: " << graph_file << std::endl;
            }
        }
        vertices = graph.get_vertices();
        std::cout << "Количество вершин: " << graph.get_vertices()
                << ", количество ребер: " << graph.get_edges().size() << std::endl;
        return 0;
    }

private:
    int vertices = 1000;
    double edge_probability = 0.3;
    Graph graph;
    std::vector<Impl> impls;

    void print_usage(const char* program_name) {
        std::cout << "Использование: " << program_name << " [опции] [файл_графа]" << std::endl;
        std::cout << "Опции:" << std::endl;
        std::cout << "  --vertices N    Количество вершин (по умолчанию 1000)" << std::endl;
        std::cout << "  --prob P        Вероятность ребра (по умолчанию 0.3)" << std::endl;
        std::cout << "  --save          Сохранить граф в файл" << std::endl;
        std::cout << "  --print         Вывести результаты" << std::endl;
        std::cout << "  --help          Показать это сообщение" << std::endl;
        std::cout << "\nЕсли файл_графа не указан, будет создан случайный граф" << std::endl;
    }
};


