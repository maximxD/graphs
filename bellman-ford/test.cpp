#include <iostream>
#include <sycl/sycl.hpp>
#include <numeric>
using namespace sycl;

int main()
{
    int counter = 0;
    buffer<int> counterBuf{ &counter, 1 };

    std::vector<int> order(1024);
    buffer<int> orderBuf{ order.data(), 1024 };
    
    queue myQueue;
    myQueue.submit([&](handler& cgh) {
        auto counterValues = counterBuf.get_access<access_mode::atomic>(cgh);
        auto orderValues = orderBuf.get_access<access_mode::write>(cgh);
                                    
        cgh.parallel_for(nd_range<1> {1024, 256}, [=](nd_item<1> it) {
            atomic_fetch_add(counterValues[0], 1);
            orderValues[it.get_global_linear_id()] = it.get_global_linear_id();
        });
    });
    myQueue.wait();

    host_accessor counter_host(counterBuf);
    std::cout << "counter: " << counter_host[0] << std::endl;

    host_accessor order_host(orderBuf);
   
    for (int i = 0; i < 1024; i++) {
        std::cout << order_host[i] << " ";
    }
    std::cout << std::endl;
    return 0;
}