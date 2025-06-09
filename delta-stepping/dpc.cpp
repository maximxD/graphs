#include <sycl/sycl.hpp>
#include <chrono>
#include <vector>
#include "task.hpp"
#include "cpp.hpp"
#include "dpc.hpp"

std::vector<int> delta_stepping_dpc_cpu(const std::vector<std::vector<std::pair<int, int>>>& adj_matrix, int source, int delta, std::chrono::duration<double>& duration) {
    q = sycl::queue(sycl::cpu_selector_v);
    return delta_stepping_dpc(adj_matrix, source, delta, duration);
}

std::vector<int> delta_stepping_dpc_gpu(const std::vector<std::vector<std::pair<int, int>>>& adj_matrix, int source, int delta, std::chrono::duration<double>& duration) {
    q = sycl::queue(sycl::gpu_selector_v);
    return delta_stepping_dpc(adj_matrix, source, delta, duration);
}

int main(int argc, char* argv[]) {
    Impl impl;
    #ifdef DPC_CPU
    impl = Impl{delta_stepping_dpc_cpu, "DPC++ CPU"};
    #else
    impl = Impl{delta_stepping_dpc_gpu, "DPC++ GPU"};
    #endif

    Task task({impl});
    // Task task({Impl{delta_stepping_cpp, "C++ W/O SET"}, impl});
    task.init(argc, argv);
    for (int i = 0; i < 10; i++) {
        task.run();
    }
    return 0;
}