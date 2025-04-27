#include <vector>
#include "../common/graph.hpp"

void bellman_ford_cpp(int vertices, std::vector<Edge> edges, int source, std::vector<int>& dist) {
    dist.assign(vertices, INF);
    dist[source] = 0;

    bool changed = true;

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

            if (dist[v] < INF && dist[v] + w < dist[u]) {
                dist[u] = dist[v] + w;
                changed = true;
            }
        }
    }
}