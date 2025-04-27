#include <iostream>
#include <vector>
#include <chrono>
#include <fstream>
#include <string>
#include <set>
#include <cstdlib>
#include "cpp.hpp"
#include "dpc.hpp"
#include "openmp.hpp"

void compare_algorithms(const Graph& graph, const std::set<std::string>& selected_impls) {
    int vertices = graph.get_vertices();
    auto edges = graph.get_edges();

    std::vector<int> dist_cpp, dist_dpc, dist_openmp;
    int source = 0;

    std::chrono::_V2::system_clock::time_point start;
    std::chrono::_V2::system_clock::time_point stop;
    std::vector<int> reference_dist; // Для хранения эталонного результата

    // Запускаем выбранные реализации
    if (selected_impls.count("cpp")) {
        start = std::chrono::high_resolution_clock::now();
        bellman_ford_cpp(vertices, edges, source, dist_cpp);
        stop = std::chrono::high_resolution_clock::now();
        auto duration_cpp = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        std::cout << "C++ реализация:    " << duration_cpp.count() << " микросекунд" << std::endl;
        reference_dist = dist_cpp;
    }

    if (selected_impls.count("dpc")) {
        start = std::chrono::high_resolution_clock::now();
        bellman_ford_dpc(vertices, edges, source, dist_dpc);
        stop = std::chrono::high_resolution_clock::now();
        auto duration_dpc = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        std::cout << "DPC++ реализация:  " << duration_dpc.count() << " микросекунд" << std::endl;
        if (reference_dist.empty()) reference_dist = dist_dpc;
    }

    if (selected_impls.count("openmp")) {
        start = std::chrono::high_resolution_clock::now();
        bellman_ford_openmp(vertices, edges, source, dist_openmp);
        stop = std::chrono::high_resolution_clock::now();
        auto duration_openmp = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        std::cout << "OpenMP реализация: " << duration_openmp.count() << " микросекунд" << std::endl;
        if (reference_dist.empty()) reference_dist = dist_openmp;
    }

    // Проверяем корректность результатов
    bool results_match = true;
    if (selected_impls.count("cpp") && dist_cpp != reference_dist) results_match = false;
    if (selected_impls.count("dpc") && dist_dpc != reference_dist) results_match = false;
    if (selected_impls.count("openmp") && dist_openmp != reference_dist) results_match = false;

    if (results_match) {
        std::cout << "Все реализации дали одинаковые результаты" << std::endl;
    } else {
        std::cout << "Ошибка: реализации дали разные результаты" << std::endl;
    }
}

void print_usage(const char* program_name) {
    std::cout << "Использование: " << program_name << " [опции] [файл_графа]" << std::endl;
    std::cout << "Опции:" << std::endl;
    std::cout << "  --cpp           Запустить C++ реализацию" << std::endl;
    std::cout << "  --dpc           Запустить DPC++ реализацию" << std::endl;
    std::cout << "  --openmp        Запустить OpenMP реализацию" << std::endl;
    std::cout << "  --all           Запустить все реализации (по умолчанию)" << std::endl;
    std::cout << "  --vertices N    Количество вершин (по умолчанию 1000)" << std::endl;
    std::cout << "  --prob P        Вероятность ребра (по умолчанию 0.3)" << std::endl;
    std::cout << "  --help          Показать это сообщение" << std::endl;
    std::cout << "\nЕсли файл_графа не указан, будет создан случайный граф" << std::endl;
}

int main(int argc, char* argv[]) {
    std::set<std::string> selected_impls;
    std::string graph_file;
    Graph graph;
    
    // Параметры по умолчанию
    int vertices = 1000;
    double edge_probability = 0.3;

    // Обработка аргументов командной строки
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--cpp") {
            selected_impls.insert("cpp");
        } else if (arg == "--dpc") {
            selected_impls.insert("dpc");
        } else if (arg == "--openmp") {
            selected_impls.insert("openmp");
        } else if (arg == "--all") {
            selected_impls.insert("cpp");
            selected_impls.insert("dpc");
            selected_impls.insert("openmp");
        } else if (arg == "--vertices" && i + 1 < argc) {
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

    // Если не выбрана ни одна реализация, используем все
    if (selected_impls.empty()) {
        selected_impls.insert("cpp");
        selected_impls.insert("dpc");
        selected_impls.insert("openmp");
    }

    // Загрузка или создание графа
    if (!graph_file.empty()) {
        try {
            graph.load_from_file(graph_file);
            std::cout << "Граф загружен из файла: " << graph_file << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Ошибка при загрузке графа: " << e.what() << std::endl;
            return 1;
        }
    } else {
        std::cout << "Создание случайного графа:" << std::endl;
        std::cout << "- Количество вершин: " << vertices << std::endl;
        std::cout << "- Вероятность ребра: " << edge_probability << std::endl;
        
        graph.create_random_graph(vertices, edge_probability);
        // graph_file = "random_graph.txt";
        // graph.save_to_file(graph_file);
        // std::cout << "Граф сохранен в файл: " << graph_file << std::endl;
    }

    std::cout << "\nСравнение реализаций алгоритма Беллмана-Форда" << std::endl;
    std::cout << "Количество вершин: " << graph.get_vertices() << std::endl;
    std::cout << "Выбранные реализации: ";
    for (const auto& impl : selected_impls) {
        std::cout << impl << " ";
    }
    std::cout << std::endl << std::endl;

    compare_algorithms(graph, selected_impls);

    return 0;
} 