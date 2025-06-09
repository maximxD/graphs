#include <sycl/sycl.hpp>
#include <vector>
#include "../common/graph.hpp"
#include "task.hpp"
#include "cpp.hpp"
#include "dpc.hpp"

std::vector<int> bellman_ford_dpc_cpu(int vertices, std::vector<Edge> edges, int source, std::chrono::duration<double>& duration) {
    sycl::queue q{sycl::cpu_selector_v};
    return bellman_ford_dpc(vertices, edges, source, duration, q);
}

std::vector<int> bellman_ford_dpc_gpu(int vertices, std::vector<Edge> edges, int source, std::chrono::duration<double>& duration) {
    sycl::queue q{sycl::gpu_selector_v};
    return bellman_ford_dpc(vertices, edges, source, duration, q);
}

int main(int argc, char* argv[]) {
    Impl impl;
    #ifdef DPC_CPU
    impl = Impl{bellman_ford_dpc_cpu, "DPC++ CPU"};
    #else
    impl = Impl{bellman_ford_dpc_gpu, "DPC++ GPU"};
    #endif

    Task task({impl});
    // Task task({Impl{bellman_ford_cpp, "C++"}, impl});
    task.init(argc, argv);
    for (int i = 0; i < 10; i++) {
        task.run();
    }
    return 0;
}