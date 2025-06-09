#include <sycl/sycl.hpp>
#include <chrono>
#include <vector>
#include "../common/graph.hpp"

sycl::queue q;

class Array {
    int *data;
    int size;

public:
    Array(int size) {
        auto data = sycl::malloc_shared<int>(size, q);
        this->size = size;

        q.submit([&](sycl::handler& h) {
            h.parallel_for(sycl::range<1>(size), [=](sycl::id<1> i) {
                data[i] = INF;
            });
        });
        q.wait();

        this->data = data;
    }

    Array(int *data, int size) {
        this->data = data;
        this->size = size;
    }

    int get_size() {
        return size;
    }

    int* get_data() {
        return data;
    }
};

class DPCBucket {
private:
    int *data;
    int size;
public:
    DPCBucket(int size) : data(sycl::malloc_shared<int>(size, q)), size(size) {}

    void insert(int index) {
        data[index] = 1;
    }

    void erase(int index) {
        data[index] = 0;
    }

    bool empty() {
        auto data = this->data;
        int *is_empty = sycl::malloc_shared<int>(1, q);
        *is_empty = 1;
        q.submit([&](sycl::handler& h) {
            h.parallel_for(sycl::range<1>(size), [=](sycl::id<1> i) {
                if (data[i] == 1) {
                    *is_empty = 0;
                }
            });
        });
        q.wait();
        bool result = *is_empty;
        sycl::free(is_empty, q);
        return result;
    }

    void union_with(const DPCBucket& other) {
        auto data = this->data;
        auto other_data = other.data;
        q.submit([&](sycl::handler& h) {
            h.parallel_for(sycl::range<1>(size), [=](sycl::id<1> i) {
                data[i] |= other_data[i];
            });
        });
        q.wait();
    }

    void clear() {
        auto data = this->data;
        q.submit([&](sycl::handler& h) {
            h.parallel_for(sycl::range<1>(size), [=](sycl::id<1> i) {
                data[i] = 0;
            });
        });
        q.wait();
    }

    Array get_vertices_array() const {
        int *vertices_count = sycl::malloc_shared<int>(1, q);
        *vertices_count = 0;

        auto vertices = this->data;
        q.submit([&](sycl::handler& h) {
            h.parallel_for(sycl::range<1>(size), [=](sycl::id<1> i) {
                if (vertices[i] == 1) {
                    sycl::atomic_ref<int, sycl::memory_order::relaxed, sycl::memory_scope::device, sycl::access::address_space::global_space>
                        atomic_vertices_count(vertices_count[0]);
                    atomic_vertices_count.fetch_add(1);
                }
            });
        });
        q.wait();

        int *current_vertices = sycl::malloc_shared<int>(*vertices_count, q);

        *vertices_count = 0;
        for (int i = 0; i < size; i++) {
            if (vertices[i] == 1) {
                current_vertices[*vertices_count] = i;
                (*vertices_count)++;
            }
        }

        auto result = Array(current_vertices, *vertices_count);
        sycl::free(vertices_count, q);
        return result;
    }
};

void relax_dpc(
    int u,
    int v,
    int weight,
    int *distances,
    DPCBucket *buckets,
    int delta
) {
    sycl::atomic_ref<int, sycl::memory_order::relaxed, sycl::memory_scope::device, sycl::access::address_space::global_space>
        atomic_distances(distances[v]);

    int old_bucket = (distances[v] == INF) ? -1 : distances[v] / delta;
    atomic_distances.fetch_min(distances[u] + weight);
    int new_bucket = distances[v] / delta;
    
    if (old_bucket != -1) {
        buckets[old_bucket].erase(v);
    }
    buckets[new_bucket].insert(v);
}


std::vector<int> delta_stepping_dpc(const std::vector<std::vector<std::pair<int, int>>>& adj_matrix, int source, int delta, std::chrono::duration<double>& duration) {
    int max_edge_weight = 100;

    int num_vertices = adj_matrix.size();
    
    int *distances = sycl::malloc_shared<int>(num_vertices, q);
    for (int i = 0; i < num_vertices; i++) {
        distances[i] = INF;
    }
    distances[source] = 0;

    int buckets_size = (max_edge_weight / delta) * 3;
    DPCBucket *buckets = sycl::malloc_shared<DPCBucket>(buckets_size, q);
    for (int i = 0; i < buckets_size; i++) {
        buckets[i] = DPCBucket(num_vertices);
    }
    buckets[0].insert(source);

    int **light_adj_matrix = sycl::malloc_shared<int*>(num_vertices, q);
    int **heavy_adj_matrix = sycl::malloc_shared<int*>(num_vertices, q);
    for (int i = 0; i < num_vertices; i++) {
        light_adj_matrix[i] = sycl::malloc_shared<int>(num_vertices, q);
        heavy_adj_matrix[i] = sycl::malloc_shared<int>(num_vertices, q);
        for (int j = 0; j < num_vertices; j++) {
            light_adj_matrix[i][j] = INF;
            heavy_adj_matrix[i][j] = INF;
        }

        for (const auto& edge : adj_matrix[i]) {
            if (edge.second < delta) {
                light_adj_matrix[i][edge.first] = edge.second;
            } else {
                heavy_adj_matrix[i][edge.first] = edge.second;
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

    // Основной цикл алгоритма
    for (int current_bucket_num = 0; current_bucket_num < buckets_size; ++current_bucket_num) {
        DPCBucket SBucket(num_vertices);
        while (!buckets[current_bucket_num].empty()) {
            DPCBucket current_bucket = buckets[current_bucket_num];
            SBucket.union_with(current_bucket);
            buckets[current_bucket_num] = DPCBucket(num_vertices);
            Array current_vertices = current_bucket.get_vertices_array();

            // Релаксация легких ребер
            auto current_vertices_data = current_vertices.get_data();
            q.submit([&](sycl::handler& h) {
                h.parallel_for(sycl::range<2>(current_vertices.get_size(), num_vertices), [=](sycl::id<2> id) {
                    int u = current_vertices_data[id[0]];
                    int v = id[1];
                    int weight = light_adj_matrix[u][v];

                    if (distances[v] > distances[u] + weight) {
                        relax_dpc(u, v, weight, distances, buckets, delta);
                    }
                });
            });
            q.wait();

            sycl::free(current_vertices_data, q);
        }

        Array current_vertices = SBucket.get_vertices_array();
        // Релаксация тяжелых ребер
        auto current_vertices_data = current_vertices.get_data();

        q.submit([&](sycl::handler& h) {
            h.parallel_for(sycl::range<2>(current_vertices.get_size(), num_vertices), [=](sycl::id<2> id) {
                int u = current_vertices_data[id[0]];
                int v = id[1];
                int weight = heavy_adj_matrix[u][v];

                if (distances[v] > distances[u] + weight) {
                    relax_dpc(u, v, weight, distances, buckets, delta);
                }
            });
        });
        q.wait();

        sycl::free(current_vertices_data, q);
    }

    auto stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::duration<double>>(stop - start);

    std::vector<int> result(distances, distances + num_vertices);

    sycl::free(distances, q);
    sycl::free(buckets, q);
    for (int i = 0; i < num_vertices; i++) {
        sycl::free(light_adj_matrix[i], q);
        sycl::free(heavy_adj_matrix[i], q);
    }
    sycl::free(light_adj_matrix, q);
    sycl::free(heavy_adj_matrix, q);

    return result;
}
