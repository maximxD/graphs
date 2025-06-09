#pragma once

#include <chrono>
#include <vector>
#include "../common/graph.hpp"

class Bucket {
private:
    std::vector<int> data;
public:
    Bucket(int size) : data(size) {}
    
    void insert(int index) {
        data[index] = 1;
    }

    void erase(int index) {
        data[index] = 0;
    }

    bool empty() const {
        for (int i = 0; i < data.size(); ++i) {
            if (data[i] == 1) {
                return false;
            }
        }
        return true;
    }

    void union_with(const Bucket& other) {
        for (int i = 0; i < data.size(); ++i) {
            data[i] |= other.data[i];
        }
    }

    void clear() {
        for (int i = 0; i < data.size(); ++i) {
            data[i] = 0;
        }
    }

    std::vector<int> get_vertices() const {
        std::vector<int> vertices;
        for (int i = 0; i < data.size(); ++i) {
            if (data[i] == 1) {
                vertices.push_back(i);
            }
        }
        return vertices;
    }
};

void relax(int u, int v, int weight, int delta, std::vector<int>& distances, std::vector<Bucket>& buckets) {
    if (distances[v] > distances[u] + weight) {
        int old_bucket = (distances[v] == INF) ? -1 : distances[v] / delta;
        distances[v] = distances[u] + weight;
        int new_bucket = distances[v] / delta;

        if (new_bucket >= buckets.size()) {
            buckets.resize(new_bucket + 1, Bucket(distances.size()));
        }
        
        if (old_bucket != -1) {
            buckets[old_bucket].erase(v);
        }
        buckets[new_bucket].insert(v);
    }
}

std::vector<int> delta_stepping_cpp(const std::vector<std::vector<std::pair<int, int>>>& adj_matrix, int source, int delta, std::chrono::duration<double>& duration) {
    int max_edge_weight = 100;

    int num_vertices = adj_matrix.size();
    
    std::vector<int> distances(num_vertices, INF);
    std::vector<Bucket> buckets(max_edge_weight / delta + 1, Bucket(num_vertices));

    std::vector<std::vector<std::pair<int, int>>> light_adj_matrix(num_vertices);
    for (int u = 0; u < adj_matrix.size(); ++u) {
        for (int v = 0; v < adj_matrix[u].size(); ++v) {
            if (adj_matrix[u][v].second < delta) {
                light_adj_matrix[u].push_back(adj_matrix[u][v]);
            }
        }
    }

    std::vector<std::vector<std::pair<int, int>>> heavy_adj_matrix(num_vertices);
    for (int u = 0; u < adj_matrix.size(); ++u) {
        for (int v = 0; v < adj_matrix[u].size(); ++v) {
            if (adj_matrix[u][v].second >= delta) {
                heavy_adj_matrix[u].push_back(adj_matrix[u][v]);
            }
        }
    }

    auto start = std::chrono::high_resolution_clock::now();

    // Проверка входных данных
    if (source < 0 || source >= num_vertices) {
        throw std::out_of_range("Source vertex is out of range");
    }
    if (delta <= 0) {
        throw std::invalid_argument("Delta must be positive");
    }

    // Инициализация
    distances[source] = 0;
    buckets[0].insert(source);

    // Основной цикл алгоритма
    for (int current_bucket_num = 0; current_bucket_num < buckets.size(); ++current_bucket_num) {
        Bucket SBucket(num_vertices);
        while (!buckets[current_bucket_num].empty()) {
            Bucket current_bucket = buckets[current_bucket_num];
            SBucket.union_with(current_bucket);
            buckets[current_bucket_num] = Bucket(num_vertices);
            std::vector<int> current_vertices = current_bucket.get_vertices();

            // Релаксация легких ребер
            for (int u : current_vertices) {
                for (const auto& edge : light_adj_matrix[u]) {
                    relax(u, edge.first, edge.second, delta, distances, buckets);
                }
            }
        }

        std::vector<int> current_vertices = SBucket.get_vertices();
        // Релаксация тяжелых ребер
        for (int u : current_vertices) {
            for (const auto& edge : heavy_adj_matrix[u]) {
                relax(u, edge.first, edge.second, delta, distances, buckets);
            }
        }
    }

    auto stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::duration<double>>(stop - start);

    return distances;
}