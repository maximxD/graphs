#include <vector>
#include <omp.h>
#include <chrono>
#include "../common/graph.hpp"

std::vector<int> bellman_ford_openmp(int vertices, std::vector<Edge> edges, int source, std::chrono::duration<double>& duration) {
    std::vector<int> dist(vertices, INF);
    dist[source] = 0;

    Edge* edges_ptr = edges.data();
    int* edges_size_ptr = new int(edges.size());
    int* dist_ptr = dist.data();
    int* changed_ptr = new int(false);

    auto duration_ptr = &duration;

    #ifdef OPENMP_GPU
    #pragma omp target data map(to:edges_ptr[:edges.size()], edges_size_ptr) map(tofrom: dist_ptr[:vertices], changed_ptr, duration_ptr)
    #endif
    {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < vertices - 1; ++i) {
            *changed_ptr = false;

            #ifdef OPENMP_CPU
            #pragma omp parallel for
            #else
            #pragma omp target teams distribute parallel for
            #endif
            for (size_t j = 0; j < *edges_size_ptr; ++j) {
                int u = edges_ptr[j].from;
                int v = edges_ptr[j].to;
                int w = edges_ptr[j].weight;

                if (dist_ptr[u] < INF && dist_ptr[u] + w < dist_ptr[v]) {
                    *changed_ptr = true;
                    #pragma omp atomic write
                    dist_ptr[v] = dist_ptr[u] + w;
                }
            }
            
            if (!*changed_ptr) {
                break;
            }
        }
        auto stop = std::chrono::high_resolution_clock::now();
        *duration_ptr = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    }

    delete edges_size_ptr;
    delete changed_ptr; 

    return dist;
}