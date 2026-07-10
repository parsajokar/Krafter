#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace Krafter {

class JobSystem {
public:
    explicit JobSystem(uint32_t threadCount = 0);
    ~JobSystem();

    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    void Dispatch(std::function<void()> job);

    size_t WorkerCount() const
    {
        return m_Workers.size();
    }

private:
    void WorkerLoop();

    std::mutex m_Mutex;
    std::condition_variable m_Cv;
    std::deque<std::function<void()>> m_Jobs;

    std::atomic<bool> m_Stop = false;
    std::vector<std::thread> m_Workers;
};

}
