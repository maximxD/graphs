#pragma once

#include <vector>
#include <random>
#include <fstream>
#include <string>
#include <stdexcept>

struct Edge {
    int from, to, weight;
};

class Graph {
private:
    std::vector<Edge> edges;
    int vertices;

public:
    // Конструктор
    Graph(int num_vertices = 0) : vertices(num_vertices) {}

    // Добавление ребра
    void add_edge(int from, int to, int weight = 1) {
        if (from < 0 || to < 0 || from >= vertices || to >= vertices) {
            throw std::out_of_range("Vertex index out of range");
        }
        edges.push_back({from, to, weight});
        // edges.push_back({to, from, weight}); // Для неориентированного графа
    }

    // Создание случайного графа
    void create_random_graph(int num_vertices, double edge_probability) {
        vertices = num_vertices;
        edges.clear();

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);
        std::uniform_int_distribution<> weight_dis(1, 100);

        for (int i = 0; i < num_vertices; ++i) {
            for (int j = i + 1; j < num_vertices; ++j) {
                if (dis(gen) < edge_probability) {
                    int weight = weight_dis(gen);
                    edges.push_back({i, j, weight});
                    // edges.push_back({j, i, weight});
                }
            }
        }
    }

    // Запись графа в файл
    void save_to_file(const std::string& filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file for writing");
        }

        file << vertices << "\n";
        for (const auto& edge : edges) {
            file << edge.from << " " << edge.to << " " << edge.weight << "\n";
        }
    }

    // Загрузка графа из файла
    void load_from_file(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file for reading");
        }

        file >> vertices;
        edges.clear();

        int from, to, weight;
        while (file >> from >> to >> weight) {
            edges.push_back({from, to, weight});
        }
    }

    // Получение количества вершин
    int get_vertices() const {
        return vertices;
    }

    // Получение всех ребер
    const std::vector<Edge>& get_edges() const {
        return edges;
    }
};

const int INF = std::numeric_limits<int>::max() / 2;