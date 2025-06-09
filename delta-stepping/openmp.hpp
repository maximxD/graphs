#include <chrono>
#include <vector>
#include <omp.h>
#include "../common/graph.hpp"

class OpenMPBucket {
private:
    int *data;
    int size;
public:
    OpenMPBucket() : size(0) {
        data = nullptr;
    }

    OpenMPBucket(int size) : size(size) {
        data = new int[size];
        for (int i = 0; i < size; i++) {
            data[i] = 0;
        }
    }

    void insert(int index) {
        data[index] = 1;
    }

    void erase(int index) {
        data[index] = 0;
    }

    bool empty() {
        bool is_empty = true;
        #pragma omp parallel for
        for (int i = 0; i < size; i++) {
            if (data[i] == 1) {
                is_empty = false;
            }
        }
        return is_empty;
    }

    void union_with(const OpenMPBucket& other) {
        #pragma omp parallel for
        for (int i = 0; i < size; i++) {
            data[i] |= other.data[i];
        }
    }

    void clear() {
        #pragma omp parallel for
        for (int i = 0; i < size; i++) {
            data[i] = 0;
        }
    }

    int* get_vertices(int& count) {
        int vertices_count = 0;
        #pragma omp parallel for
        for (int i = 0; i < size; i++) {
            if (data[i] == 1) {
                #pragma omp atomic
                vertices_count++;
            }
        }
        count = vertices_count;

        int *current_vertices = new int[vertices_count];
        vertices_count = 0;
        for (int i = 0; i < size; i++) {
            if (data[i] == 1) {
                current_vertices[vertices_count] = i;
                vertices_count++;
            }
        }

        return current_vertices;
    }
};

void relax_openmp(
    int u,
    int v,
    int weight,
    int *distances,
    OpenMPBucket *buckets,
    int delta
) {
    int old_bucket = (distances[v] == INF) ? -1 : distances[v] / delta;

    #pragma omp atomic compare
    if (distances[v] > distances[u] + weight) {
        distances[v] = distances[u] + weight;
    }

    int new_bucket = distances[v] / delta;
    if (old_bucket != -1) {
        buckets[old_bucket].erase(v);
    }
    buckets[new_bucket].insert(v);
}

std::vector<int> delta_stepping_openmp(const std::vector<std::vector<std::pair<int, int>>>& adj_matrix, int source, int delta, std::chrono::duration<double>& duration) {
    int max_edge_weight = 100;
    int num_vertices = adj_matrix.size();
    
    int *distances = new int[num_vertices];
    for (int i = 0; i < num_vertices; i++) {
        distances[i] = INF;
    }
    distances[source] = 0;

    int buckets_size = (max_edge_weight / delta + 1) * 3;
    OpenMPBucket *buckets = new OpenMPBucket[buckets_size];
    for (int i = 0; i < buckets_size; i++) {
        buckets[i] = OpenMPBucket(num_vertices);
    }
    buckets[0].insert(source);

    int **light_adj_matrix = new int*[num_vertices];
    int **heavy_adj_matrix = new int*[num_vertices];
    for (int i = 0; i < num_vertices; i++) {
        light_adj_matrix[i] = new int[num_vertices];
        heavy_adj_matrix[i] = new int[num_vertices];
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


    if (source < 0 || source >= num_vertices) {
        throw std::out_of_range("Source vertex is out of range");
    }
    if (delta <= 0) {
        throw std::invalid_argument("Delta must be positive");
    }

    #ifdef OPENMP_GPU
    #pragma omp target data map(to:buckets[:buckets_size], light_adj_matrix[:num_vertices][:num_vertices], heavy_adj_matrix[:num_vertices][:num_vertices]) map(tofrom: distances[:num_vertices])
    #endif
    {
        auto start = std::chrono::high_resolution_clock::now();
        for (int current_bucket_num = 0; current_bucket_num < buckets_size; ++current_bucket_num) {
            OpenMPBucket SBucket(num_vertices);
            while (!buckets[current_bucket_num].empty()) {
                OpenMPBucket current_bucket = buckets[current_bucket_num];
                SBucket.union_with(current_bucket);
                buckets[current_bucket_num] = OpenMPBucket(num_vertices);

                int current_vertices_count;
                int *current_vertices_data = current_bucket.get_vertices(current_vertices_count);

                #ifdef OPENMP_GPU
                #pragma omp target teams distribute parallel for collapse(2)
                #else
                #pragma omp parallel for collapse(2)
                #endif
                for (int i = 0; i < current_vertices_count; i++) {
                    for (int j = 0; j < num_vertices; j++) {
                        int u = current_vertices_data[i];
                        int v = j;
                        int weight = light_adj_matrix[u][v];

                        if (distances[v] > distances[u] + weight) {
                            relax_openmp(u, v, weight, distances, buckets, delta);
                        }
                    }
                }
            }

            int current_vertices_count;
            int *current_vertices_data = SBucket.get_vertices(current_vertices_count);

            #ifdef OPENMP_GPU
            #pragma omp target teams distribute parallel for collapse(2)
            #else
            #pragma omp parallel for collapse(2)
            #endif
            for (int i = 0; i < current_vertices_count; i++) {
                for (int j = 0; j < num_vertices; j++) {
                    int u = current_vertices_data[i];
                    int v = j;
                    int weight = heavy_adj_matrix[u][v];

                    if (distances[v] > distances[u] + weight) {
                        relax_openmp(u, v, weight, distances, buckets, delta);
                    }
                }
            }
        }

        auto stop = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::duration<double>>(stop - start);
    }

    std::vector<int> result(distances, distances + num_vertices);

    delete[] distances;
    delete[] buckets;
    for (int i = 0; i < num_vertices; i++) {
        delete[] light_adj_matrix[i];
        delete[] heavy_adj_matrix[i];
    }
    delete[] light_adj_matrix;
    delete[] heavy_adj_matrix;

    return result;
}
