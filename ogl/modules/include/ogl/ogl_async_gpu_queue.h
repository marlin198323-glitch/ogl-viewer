#ifndef OGL_ASYNC_GPU_QUEUE_H
#define OGL_ASYNC_GPU_QUEUE_H

// Bounded, latest-wins diagnostic pipeline. Uses no Vulkan or viewer headers.
// Never invokes Firestorm logging or OpenGL on the worker thread.
// Does NOT make Vulkan builds persistent, nor render RT pixels in the viewer.
#include "ogl_rtx_api.h"
#include <array>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace ogl {
class AsyncGpuQueue {
public:
    struct Job {
        uint64_t revision = 0;          // Unique asynchronous job sequence
        uint64_t geometry_revision = 0; // OpenSim mesh snapshot revision
        std::vector<ogl_rtx_triangle> triangles;
        std::array<float, 3> sun{{0.f, 0.f, 1.f}};
        std::array<float, 3> camera_position{{0.f, 0.f, 0.f}};
        std::array<float, 3> camera_forward{{0.f, 0.f, 0.f}};
        std::array<float, 3> camera_right{{0.f, 0.f, 0.f}};
        std::array<float, 3> camera_up{{0.f, 0.f, 0.f}};
        float vertical_fov_radians = 0.f;
        float aspect_ratio = 0.f;
        float near_clip_m = 0.f;
        float far_clip_m = 0.f;
        std::array<float,64> depth_grid{};
        bool has_depth_grid = false;
        uint32_t surface_pixels = 0;
        uint32_t background_pixels = 0;
        uint32_t viewport_width = 0;
        uint32_t viewport_height = 0;
    };
    struct Result {
        uint64_t revision = 0;
        uint64_t geometry_revision = 0;
        uint32_t triangle_count = 0;
        uint32_t ray_count = 0;
        uint32_t occluded = 0;
        uint32_t primary_hits = 0;
        uint32_t primary_depth_matches = 0;
        uint32_t primary_closer = 0;
        uint32_t primary_farther = 0;
        uint32_t surface_pixels = 0;
        uint32_t background_pixels = 0;
        std::array<uint8_t, 64> shadow_mask{};
        std::array<uint8_t, 64> surface_mask{};
        uint32_t gpu_device_creations = 0;
        uint32_t blas_builds = 0;
        uint32_t blas_refits = 0;
        uint32_t tlas_refits = 0;
        uint32_t cache_hits = 0;
        uint32_t wall_ms = 0; // end-to-end Vulkan worker wall time, not GPU timer
        int return_code = 1;
    };
    struct Stats {
        uint64_t attempts = 0;
        uint64_t successes = 0;
        uint64_t replacements = 0;
        bool disabled = false;
        bool busy = false;
    };
    using Work = std::function<Result(const Job&)>;

    explicit AsyncGpuQueue(Work work)
        : m_work(std::move(work)), m_thread(&AsyncGpuQueue::run, this) {}
    ~AsyncGpuQueue() { stop(); }
    AsyncGpuQueue(const AsyncGpuQueue&) = delete;
    AsyncGpuQueue& operator=(const AsyncGpuQueue&) = delete;

    bool enqueue(Job job) {
        if (job.revision == 0 || job.triangles.empty() ||
            job.triangles.size() > OGL_RTX_MAX_SCENE_TRIANGLES) return false;
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stop || m_stats.disabled || job.revision <= m_latest_revision)
            return false;
        m_latest_revision = job.revision;
        if (!m_pending.triangles.empty()) ++m_stats.replacements;
        m_pending = std::move(job); // at most one pending snapshot, no backlog
        m_cv.notify_one();
        return true;
    }

    // A teleport clears pending work AND makes a running GPU result obsolete.
    // A running Vulkan call cannot be cancelled and may finish after this.
    void invalidate(uint64_t revision) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (revision > m_latest_revision) m_latest_revision = revision;
        m_min_valid_revision = m_latest_revision;
        m_pending = {};
        m_completed = {};
        m_has_completed = false;
    }

    bool try_take(Result& output) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_has_completed) return false;
        output = m_completed;
        m_has_completed = false;
        return true;
    }

    Stats stats() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        Stats result = m_stats;
        result.busy = m_busy || !m_pending.triangles.empty();
        return result;
    }

    // Stop joins the opt-in GPU worker BEFORE unloading its Vulkan DLL/SO.
    // A driver hang can delay shutdown; never use this in public mode yet.
    void stop() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stop = true;
            m_pending = {};
            m_completed = {};
            m_has_completed = false;
            m_cv.notify_all();
        }
        if (m_thread.joinable()) m_thread.join();
    }

private:
    void run() {
        std::unique_lock<std::mutex> lock(m_mutex);
        for (;;) {
            m_cv.wait(lock, [this] {
                return m_stop || !m_pending.triangles.empty();
            });
            if (m_stop) return;
            Job job = std::move(m_pending);
            m_pending = {};
            m_busy = true;
            ++m_stats.attempts;
            lock.unlock();
            Result outcome{};
            try { outcome = m_work(job); }
            catch (...) { outcome.return_code = 1; }
            lock.lock();
            m_busy = false;
            // Never publish stale results after region changes.
            if (!m_stop && job.revision > m_min_valid_revision &&
                job.revision >= m_latest_revision) {
                uint32_t mask_hits = 0;
                bool mask_valid = true;
                for (uint8_t pixel : outcome.shadow_mask) {
                    if (pixel > 1u) mask_valid = false;
                    mask_hits += pixel;
                }
                if (outcome.return_code != 0 || outcome.ray_count != 64 ||
                    outcome.triangle_count != job.triangles.size() ||
                    outcome.occluded > outcome.ray_count || !mask_valid ||
                    mask_hits != outcome.occluded) {
                    m_stats.disabled = true; // fail closed until next start
                    m_pending = {};
                } else {
                    ++m_stats.successes;
                }
                outcome.revision = job.revision;
                outcome.geometry_revision = job.geometry_revision;
                m_completed = outcome;
                m_has_completed = true;
            }
        }
    }
    Work m_work;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    Job m_pending{};
    Result m_completed{};
    Stats m_stats{};
    bool m_has_completed = false;
    bool m_busy = false;
    bool m_stop = false;
    uint64_t m_latest_revision = 0;
    uint64_t m_min_valid_revision = 0;
    std::thread m_thread;
};
} // namespace ogl
#endif
