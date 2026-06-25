#include "Krafter/Core/JobSystem.h"

namespace Krafter {

JobSystem::JobSystem(uint32_t threadCount)
{
    if (threadCount == 0) {
        uint32_t hardware = std::thread::hardware_concurrency();
        threadCount = hardware > 1 ? hardware - 1 : 1;
    }

    m_Workers.reserve(threadCount);
    for (uint32_t i = 0; i < threadCount; i++) {
        m_Workers.emplace_back([this] { WorkerLoop(); });
    }
}

JobSystem::~JobSystem()
{
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Stop = true;
    }
    m_Cv.notify_all();
    for (auto& worker : m_Workers) {
        worker.join();
    }
}

void JobSystem::Dispatch(std::function<void()> job)
{
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Jobs.push_back(std::move(job));
    }
    m_Cv.notify_one();
}

void JobSystem::WorkerLoop()
{
    while (true) {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(m_Mutex);
            m_Cv.wait(lock, [this] { return m_Stop.load() || !m_Jobs.empty(); });
            if (m_Stop.load() && m_Jobs.empty()) {
                return;
            }
            job = std::move(m_Jobs.front());
            m_Jobs.pop_front();
        }
        job();
    }
}

} // namespace Krafter
