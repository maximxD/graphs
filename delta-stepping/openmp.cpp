#include "task.hpp"
#include "cpp.hpp"
#include "openmp.hpp"

int main(int argc, char* argv[]) {
    Impl impl{delta_stepping_openmp, "OpenMP"};
    Task task({impl});
    // Task task({Impl{delta_stepping, "C++ W/O SET"}, impl});
    task.init(argc, argv);
    for (int i = 0; i < 10; i++) {
        task.run();
    }
    return 0;
} 