#ifndef OGL_DYNAMIC_SCENE_H
#define OGL_DYNAMIC_SCENE_H

#include "ogl_scene_validation.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <vector>

// Prototype of stable per-object acceleration metadata. The actual Vulkan
// scene still uses a single BLAS/TLAS instance; this manager never claims
// to upload all OpenSim objects automatically. No GL/Vulkan dependency.
namespace ogl {
using ObjectKey = std::array<uint8_t, 16>; // complete OpenSim UUID
using Affine3x4 = std::array<float, 12>;    // row-major local-to-world
enum class SceneChange : uint8_t {
    Invalid, Unchanged, NewInstance, GeometryChanged, TransformChanged, Removed
};
struct SceneEntry {
    ObjectKey key{};
    std::vector<ogl_rtx_triangle> local_geometry;
    Affine3x4 transform{};
    uint64_t geometry_revision = 0;
    uint64_t transform_revision = 0;
};
class DynamicScene {
public:
    static constexpr uint32_t kMaxObjects = 1024;
    static constexpr uint32_t kMaxTriangles = OGL_RTX_MAX_SCENE_TRIANGLES;
    SceneChange upsert(const ObjectKey& id, const ogl_rtx_triangle* triangles,
                       uint32_t count, const Affine3x4& transform) {
        if (!count || !triangles || count > kMaxTriangles ||
            !valid_transform(transform)) return SceneChange::Invalid;
        for (uint32_t i=0; i<count; ++i)
            if (!is_valid_triangle(triangles[i])) return SceneChange::Invalid;
        const auto it = m_objects.find(id);
        const bool is_new = it == m_objects.end();
        if (is_new && m_objects.size() >= kMaxObjects)
            return SceneChange::Invalid;
        const size_t old_count = is_new ? 0 : it->second.local_geometry.size();
        if (m_triangle_count - old_count + count > kMaxTriangles)
            return SceneChange::Invalid;
        if (is_new) {
            SceneEntry entry{};
            entry.key = id;
            entry.local_geometry.assign(triangles,triangles+count);
            entry.transform=transform;
            entry.geometry_revision=++m_geometry_generation;
            entry.transform_revision=++m_transform_generation;
            m_objects.emplace(id,std::move(entry));
            m_triangle_count+=count;
            return SceneChange::NewInstance;
        }
        SceneEntry& entry=it->second;
        const bool same_geometry=old_count==count &&
            std::memcmp(entry.local_geometry.data(),triangles,
                        size_t(count)*sizeof(*triangles))==0;
        const bool same_transform=entry.transform==transform;
        if (same_geometry && same_transform) return SceneChange::Unchanged;
        if (!same_geometry) {
            entry.local_geometry.assign(triangles,triangles+count);
            m_triangle_count=m_triangle_count-old_count+count;
            entry.geometry_revision=++m_geometry_generation;
        }
        if (!same_transform) {
            entry.transform=transform;
            entry.transform_revision=++m_transform_generation;
        }
        return !same_geometry ? SceneChange::GeometryChanged :
                                SceneChange::TransformChanged;
    }
    SceneChange update_transform(const ObjectKey& id,
                                  const Affine3x4& transform) {
        auto it=m_objects.find(id);
        if (it==m_objects.end() || !valid_transform(transform))
            return SceneChange::Invalid;
        if (it->second.transform==transform) return SceneChange::Unchanged;
        it->second.transform=transform;
        it->second.transform_revision=++m_transform_generation;
        return SceneChange::TransformChanged;
    }
    SceneChange erase(const ObjectKey& id) {
        auto it=m_objects.find(id);
        if (it==m_objects.end()) return SceneChange::Unchanged;
        m_triangle_count-=it->second.local_geometry.size();
        m_objects.erase(it);
        ++m_geometry_generation;
        ++m_transform_generation;
        return SceneChange::Removed;
    }
    void clear() {
        if (m_objects.empty()) return;
        m_objects.clear();
        m_triangle_count=0;
        ++m_geometry_generation;
        ++m_transform_generation;
    }
    const SceneEntry* find(const ObjectKey& id) const {
        auto it=m_objects.find(id);
        return it==m_objects.end()? nullptr:&it->second;
    }
    size_t objects() const {return m_objects.size();}
    size_t triangles() const {return m_triangle_count;}
    uint64_t geometry_generation() const {return m_geometry_generation;}
    uint64_t transform_generation() const {return m_transform_generation;}
    template<class Fn> void visit(Fn&& fn) const {
        for (const auto& item:m_objects) fn(item.second);
    }
    static bool valid_transform(const Affine3x4& t) {
        for (float v:t) if (!std::isfinite(v) || std::fabs(v)>1000000.f)
            return false;
        // Non-singular 3x3 transformation (no collapsed/invalid instances).
        const float det =
             t[0]*(t[5]*t[10]-t[6]*t[9])
            -t[1]*(t[4]*t[10]-t[6]*t[8])
            +t[2]*(t[4]*t[9]-t[5]*t[8]);
        return std::isfinite(det) && std::fabs(det)>1e-7f;
    }
private:
    std::map<ObjectKey,SceneEntry> m_objects;
    size_t m_triangle_count=0;
    uint64_t m_geometry_generation=0;
    uint64_t m_transform_generation=0;
};
} // namespace ogl
#endif
