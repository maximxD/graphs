#include <sycl/sycl.hpp>
#include <vector>
#include "../common/graph.hpp"

using namespace sycl;

void bellman_ford_dpc(int vertices, std::vector<Edge> edges, int source, std::vector<int>& dist) {
    dist.assign(vertices, INF);
    dist[source] = 0;

    queue q{gpu_selector_v};

    int *dist_device = malloc_device<int>(vertices, q);
    q.memcpy(dist_device, dist.data(), sizeof(int) * vertices);

    Edge *edges_device = malloc_device<Edge>(edges.size(), q);
    q.memcpy(edges_device, edges.data(), sizeof(Edge) * edges.size());

    int* changed = malloc_shared<int>(1, q);

    int* edges_size = malloc_shared<int>(1, q);
    *edges_size = edges.size();

    for (int i = 0; i < vertices - 1; ++i) {
        changed[0] = 0;

        q.submit([&](handler& h) {
            h.parallel_for(range<1>(edges.size() * 2), [=](id<1> idx) {
                int i = idx % *edges_size;
                int u = edges_device[i].from;
                int v = edges_device[i].to;
                int w = edges_device[i].weight;

                if (idx > *edges_size) {
                    u = edges_device[i].to;
                    v = edges_device[i].from;
                }

                if (dist_device[u] < INF && dist_device[u] + w < dist_device[v]) {
                    changed[0] = 1;

                    atomic_ref<int, memory_order::relaxed, memory_scope::device, access::address_space::global_space>
                        atomic_dist_v(dist_device[v]);
                    int old = atomic_dist_v.load();
                    int new_val = dist_device[u] + w;
                    while (old > new_val && !atomic_dist_v.compare_exchange_strong(old, new_val)) {}
                }
            });
        });
        q.wait();

        if (changed[0] == 0) {
            break;
        }
    }

    q.memcpy(dist.data(), dist_device, sizeof(int) * vertices);

    free(edges_device, q);
    free(dist_device, q);
    free(changed, q);
}