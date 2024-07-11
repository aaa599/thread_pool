# thread_pool
使用C++实现的简易线程池
# 使用方式
        std::mutex _m;
        int main() {
            {
                ThreadPool threadPool(5);
                for (int i = 0; i < 20; ++i) {
                    threadPool.submit([i](){
                        std::unique_lock<std::mutex> lock(_m);
                        std::cout << i << std::endl;
                    });
                }
            }
            //    {
            //        ThreadPool pool(8);
            //        int n = 20;
            //        for (int i = 1; i <= n; i++) {
            //            pool.submit([](int id) {
            //                if (id % 2 == 1) {
            //                    std::this_thread::sleep_for(std::chrono::seconds(1));
            //                }
            //                std::unique_lock<std::mutex> lc(_m);
            //                std::cout << "id : " << id << std::endl;
            //            }, i);
            //        }
            //    }
            return 0;
        }
