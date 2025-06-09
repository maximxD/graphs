#include <vector>
#include <omp.h>
#include "../common/graph.hpp"
#include "task.hpp"
#include "cpp.hpp"
#include "openmp.hpp"

int main(int argc, char* argv[]) {
    Impl impl;
    #ifdef OPENMP_CPU
    impl = Impl{bellman_ford_openmp, "OpenMP CPU"};
    #else
    impl = Impl{bellman_ford_openmp, "OpenMP GPU"};
    #endif

    Task task({impl});
    // Task task({Impl{bellman_ford_cpp, "C++"}, impl});
    task.init(argc, argv);
    for (int i = 0; i < 10; i++) {  
        task.run();
    }
    return 0;
}
