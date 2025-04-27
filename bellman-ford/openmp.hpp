#include <vector>
#include <omp.h>
#include <iostream>
#include "../common/graph.hpp"

void bellman_ford_openmp(int vertices, std::vector<Edge> edges, int source, std::vector<int>& dist) {
    dist.assign(vertices, INF);
    dist[source] = 0;

    for (int i = 0; i < vertices - 1; ++i) {
        bool changed = false;
        int edges_size = edges.size();

        #pragma omp target teams distribute parallel for map(to: edges[:edges_size]) map(tofrom: dist[:vertices], changed, edges_size)
        for (size_t j = 0; j < edges_size * 2; ++j) {
            int n = j % edges_size;

            int u = edges[n].from;
            int v = edges[n].to;
            int w = edges[n].weight;

            if (j >= edges_size) {
                u = edges[n].to;
                v = edges[n].from;
            }

            if (dist[u] < INF && dist[u] + w < dist[v]) {
                #pragma omp atomic write
                dist[v] = dist[u] + w;
                #pragma omp atomic write
                changed = true;
            }
        }
        
        if (!changed) {
            break;
        }
    }
}
