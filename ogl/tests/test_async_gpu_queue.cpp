// Deterministic, GPU-independent tests for bounded async RT diagnostics.
#include "ogl/ogl_async_gpu_queue.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "Async test FAIL at %s:%d: %s\n", \
                 __FILE__, __LINE__, #condition); std::abort(); \
} } while (0)

using namespace std::chrono_literals;
using Queue = ogl::AsyncGpuQueue;

Queue::Job make_job(uint64_t revision) {
    Queue::Job job{};
    job.revision = revision;
    job.triangles.resize(1);
    job.triangles[0].position[3] = 1.f;
    job.triangles[0].position[7] = 1.f;
    return job;
}
Queue::Result completed(const Queue::Job& job, int code = 0) {
    Queue::Result r{};
    r.revision = job.revision;
    r.triangle_count = static_cast<uint32_t>(job.triangles.size());
    r.ray_count = 64;
    r.occluded = 15;
    r.return_code = code;
    return r;
}

template <typename Condition>
bool wait_for(Condition check) {
    const auto limit = std::chrono::steady_clock::now() + 3s;
    while (std::chrono::steady_clock::now() < limit) {
        if (check()) return true;
        std::this_thread::sleep_for(2ms);
    }
    return false;
}

int main() {
    {
        // A slow GPU does not block submitters; only one pending job is kept.
        std::mutex gate_mutex;
        std::condition_variable gate_cv;
        std::atomic<bool> release{false};
        std::atomic<uint64_t> running{0};
        Queue queue([&](const Queue::Job& job) {
            running.store(job.revision);
            std::unique_lock<std::mutex> lock(gate_mutex);
            gate_cv.wait(lock, [&] { return release.load(); });
            return completed(job);
        });
        const auto begin = std::chrono::steady_clock::now();
        CHECK(queue.enqueue(make_job(1)));
        CHECK(std::chrono::steady_clock::now() - begin < 100ms);
        CHECK(wait_for([&]{return running.load() == 1;}));
        CHECK(queue.enqueue(make_job(2)));
        CHECK(queue.enqueue(make_job(3)));
        CHECK(queue.stats().replacements == 1);
        CHECK(!queue.enqueue(make_job(3))); // deduped / no rollback
        Queue::Result out{};
        CHECK(!queue.try_take(out)); // worker still blocked
        release.store(true);
        gate_cv.notify_all();
        CHECK(wait_for([&]{return queue.stats().successes == 1;}));
        CHECK(queue.stats().attempts == 2); // 1 obsolete + 3 newest
        CHECK(queue.try_take(out));
        CHECK(out.revision == 3 && out.occluded == 15);
        CHECK(!queue.try_take(out));
        CHECK(!queue.stats().disabled);
    }
    {
        // Teleport/region clear invalidates an in-flight GPU result.
        std::mutex gate_mutex;
        std::condition_variable gate_cv;
        std::atomic<bool> release{false};
        std::atomic<bool> working{false};
        Queue queue([&](const Queue::Job& job) {
            working.store(true);
            std::unique_lock<std::mutex> lock(gate_mutex);
            gate_cv.wait(lock, [&]{return release.load();});
            return completed(job);
        });
        CHECK(queue.enqueue(make_job(4)));
        CHECK(wait_for([&]{return working.load();}));
        queue.invalidate(5);
        CHECK(!queue.enqueue(make_job(5)));
        release.store(true);
        gate_cv.notify_all();
        CHECK(wait_for([&]{return queue.stats().attempts == 1 &&
                                  !queue.stats().busy;}));
        Queue::Result out{};
        CHECK(!queue.try_take(out)); // revision 4 never published
        CHECK(queue.enqueue(make_job(6)));
        CHECK(wait_for([&]{return queue.stats().successes == 1;}));
        CHECK(queue.try_take(out) && out.revision == 6);
    }
    {
        // Fail-closed on driver/probe error. Public OpenGL remains unaffected.
        Queue queue([](const Queue::Job& job) { return completed(job, 1); });
        CHECK(queue.enqueue(make_job(1)));
        CHECK(wait_for([&]{return queue.stats().disabled;}));
        CHECK(queue.stats().attempts == 1);
        CHECK(queue.stats().successes == 0);
        CHECK(!queue.enqueue(make_job(2)));
        Queue::Result result{};
        CHECK(queue.try_take(result) && result.return_code == 1);
    }
    {
        // Explicit stop releases the worker and rejects future submissions.
        Queue queue([](const Queue::Job& job){ return completed(job); });
        queue.stop();
        CHECK(!queue.enqueue(make_job(1)));
    }
    {
        // Empty and excessive snapshots never reach the GPU thread.
        Queue queue([](const Queue::Job& job){ return completed(job); });
        auto empty = make_job(1);
        empty.triangles.clear();
        CHECK(!queue.enqueue(std::move(empty)));
        auto excessive = make_job(2);
        excessive.triangles.resize(OGL_RTX_MAX_SCENE_TRIANGLES + 1);
        CHECK(!queue.enqueue(std::move(excessive)));
        CHECK(queue.stats().attempts == 0);
    }
}
