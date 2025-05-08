#include <vector>
#include "../common/graph.hpp"
#include "task.hpp"

std::vector<int> bellman_ford_cpp(int vertices, std::vector<Edge> edges, int source, std::chrono::duration<double>& duration) {
    std::vector<int> dist(vertices, INF);
    dist[source] = 0;

    bool changed = true;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < vertices - 1 && changed; ++i) {
        changed = false;
        for (const auto& edge : edges) {
            int u = edge.from;
            int v = edge.to;
            int w = edge.weight;
            if (dist[u] < INF && dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                changed = true;
            }
        }

        if (!changed) {
            break;
        }
    }
    auto stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

    // for (int i = 0; i < vertices; i++) {
    //     if (dist[i] == INF) {
    //         std::cout << "INF ";
    //     } else {
    //         std::cout << dist[i] << " ";
    //     }
    // }
    // std::cout << std::endl;
    return dist;
}
