#include <vector>
#include "../common/graph.hpp"
#include "task.hpp"
#include "cpp.hpp"

int main(int argc, char* argv[]) {
    Task task({Impl{bellman_ford_cpp, "C++"}});
    // Task task({Impl{bellman_ford_cpp, "C++"}, Impl{bellman_ford_cpp, "C++"}});
    task.init(argc, argv);
    for (int i = 0; i < 10; i++) {
        task.run();
    }
    return 0;
}
