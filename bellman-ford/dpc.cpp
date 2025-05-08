#include <sycl/sycl.hpp>
#include <vector>
#include "../common/graph.hpp"
#include "task.hpp"
#include "cpp.hpp"

using namespace sycl;

std::vector<int> bellman_ford_dpc(int vertices, std::vector<Edge> edges, int source, std::chrono::duration<double>& duration) {
    queue q{gpu_selector_v};

    int *dist_device = malloc_device<int>(vertices, q);
    q.fill(dist_device, INF, vertices);
    q.memset(dist_device + source, 0, sizeof(int));

    Edge *edges_device = malloc_device<Edge>(edges.size(), q);
    q.memcpy(edges_device, edges.data(), sizeof(Edge) * edges.size());

    int* changed = malloc_shared<int>(1, q);

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < vertices - 1; ++i) {
        changed[0] = 0;

        q.submit([&](handler& h) {
            h.parallel_for(range<1>(edges.size()), [=](id<1> idx) {
                int u = edges_device[idx].from;
                int v = edges_device[idx].to;
                int w = edges_device[idx].weight;

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
    auto stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

    std::vector<int> dist(vertices);
    q.memcpy(dist.data(), dist_device, sizeof(int) * vertices);

    free(edges_device, q);
    free(dist_device, q);
    free(changed, q);
    return dist;
}

int main(int argc, char* argv[]) {
    // Task task({Impl{bellman_ford_cpp, "C++"}, Impl{bellman_ford_dpc, "DPC++"}});
    Task task({Impl{bellman_ford_dpc, "DPC++"}});
    task.init(argc, argv);
    for (int i = 0; i < 10; i++) {
        task.run();
    }
    return 0;
}