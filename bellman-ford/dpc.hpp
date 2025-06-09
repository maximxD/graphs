#include <sycl/sycl.hpp>
#include <vector>
#include "../common/graph.hpp"

std::vector<int> bellman_ford_dpc(int vertices, std::vector<Edge> edges, int source, std::chrono::duration<double>& duration, sycl::queue q) {
    std::vector<int> dist(vertices, INF);
    dist[source] = 0;

    int *dist_device = sycl::malloc_device<int>(vertices, q);
    q.memcpy(dist_device, dist.data(), sizeof(int) * vertices);

    Edge *edges_device = sycl::malloc_device<Edge>(edges.size(), q);
    q.memcpy(edges_device, edges.data(), sizeof(Edge) * edges.size());

    q.wait();

    int* changed = sycl::malloc_shared<int>(1, q);
    changed[0] = 1;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < vertices - 1 && changed[0] == 1; ++i) {
        q.submit([&](sycl::handler& h) {
            changed[0] = 0;

            h.parallel_for(sycl::range<1>(edges.size()), [=](sycl::id<1> idx) {
                int u = edges_device[idx].from;
                int v = edges_device[idx].to;
                int w = edges_device[idx].weight;

                if (dist_device[u] < INF && dist_device[u] + w < dist_device[v]) {
                    changed[0] = 1;

                    sycl::atomic_ref<int, sycl::memory_order::relaxed, sycl::memory_scope::device, sycl::access::address_space::global_space>
                        atomic_dist_v(dist_device[v]);
                    int old = atomic_dist_v.load();
                    int new_val = dist_device[u] + w;
                    while (old > new_val && !atomic_dist_v.compare_exchange_strong(old, new_val)) {}
                }
            });
        });
        q.wait();
    }
    auto stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

    q.memcpy(dist.data(), dist_device, sizeof(int) * vertices);
    q.wait();

    sycl::free(edges_device, q);
    sycl::free(dist_device, q);
    sycl::free(changed, q);
    return dist;
}
