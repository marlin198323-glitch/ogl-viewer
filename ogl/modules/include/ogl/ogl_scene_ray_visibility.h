#ifndef OGL_SCENE_RAY_VISIBILITY_H
#define OGL_SCENE_RAY_VISIBILITY_H

// Device-independent, developer-only camera-ray geometry selection.
// Finds exact mesh triangles under the same 8x8 ray centres as the Vulkan
// depth probe, without assuming that sparsely sampled mesh faces are solid.
#include "ogl_scene_validation.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <cstring>
#include <vector>

namespace ogl {
struct SceneRay {
    std::array<float,3> origin{};
    std::array<float,3> direction{};
};
struct SceneRayHit {
    float distance = std::numeric_limits<float>::infinity();
    ogl_rtx_triangle triangle{};
};
using SceneRays = std::array<SceneRay,64>;
using SceneRayHits = std::array<SceneRayHit,64>;

inline SceneRays camera_probe_rays(const float* origin, const float* forward,
    const float* right, const float* up, float fov_y, float aspect) {
    SceneRays rays{};
    const float scale=std::tan(0.5f*fov_y);
    for (unsigned y=0;y<8;++y) for (unsigned x=0;x<8;++x) {
        auto& ray=rays[y*8+x];
        const float screen_x=((float(x)+0.5f)/4.f-1.f)*scale*aspect;
        const float screen_y=((float(y)+0.5f)/4.f-1.f)*scale;
        float len2=0.f;
        for (unsigned a=0;a<3;++a) {
            ray.origin[a]=origin[a];
            ray.direction[a]=forward[a]+screen_x*right[a]+screen_y*up[a];
            len2+=ray.direction[a]*ray.direction[a];
        }
        const float inv=1.f/std::sqrt(len2);
        for (float& c : ray.direction) c*=inv;
    }
    return rays;
}

inline bool ray_hits_sphere(const SceneRay& ray, const float* centre,
                            float radius) {
    if (!(radius>0.f) || !std::isfinite(radius)) return false;
    float forward_distance=0.f, centre_distance2=0.f;
    for (unsigned a=0;a<3;++a) {
        const float delta=centre[a]-ray.origin[a];
        forward_distance+=delta*ray.direction[a];
        centre_distance2+=delta*delta;
    }
    // Include rays whose origin is already inside a bounding sphere.
    if (forward_distance < -radius) return false;
    const float perpendicular_sq=std::max(0.f,centre_distance2-
                                            forward_distance*forward_distance);
    return perpendicular_sq <= radius*radius;
}

// Conservative world-space bounds from the drawable (rather than its
// nominal object scale). Large/displaced OpenSim meshes can have geometry
// away from the object's render position. No mesh traversal for a missed box.
inline bool ray_aabb_entry_distance(const SceneRay& ray, const float* low,
                                    const float* high, float& entry,
                                    float padding=0.5f) {
    if (!low || !high || !std::isfinite(padding) || padding<0.f)
        return false;
    float closest=0.f, furthest=1000.f;
    for (unsigned axis=0;axis<3;++axis) {
        if (!std::isfinite(low[axis]) || !std::isfinite(high[axis]) ||
            low[axis]>high[axis] ||
            !std::isfinite(ray.origin[axis]) ||
            !std::isfinite(ray.direction[axis]))
            return false;
        const float lo=low[axis]-padding;
        const float hi=high[axis]+padding;
        if (std::fabs(ray.direction[axis]) < 1.e-7f) {
            if (ray.origin[axis]<lo || ray.origin[axis]>hi) return false;
            continue;
        }
        float t0=(lo-ray.origin[axis])/ray.direction[axis];
        float t1=(hi-ray.origin[axis])/ray.direction[axis];
        if (t0>t1) std::swap(t0,t1);
        closest=std::max(closest,t0);
        furthest=std::min(furthest,t1);
        if (furthest<closest) return false;
    }
    if (furthest<0.f) return false;
    entry=closest;
    return true;
}

inline bool ray_hits_aabb(const SceneRay& ray, const float* low,
                          const float* high, float padding=0.5f) {
    float ignored=0.f;
    return ray_aabb_entry_distance(ray,low,high,ignored,padding);
}

inline bool ray_hits_triangle(const SceneRay& ray, const ogl_rtx_triangle& tri,
                              float& distance) {
    const float* v=tri.position;
    const float a[3]{v[3]-v[0],v[4]-v[1],v[5]-v[2]};
    const float b[3]{v[6]-v[0],v[7]-v[1],v[8]-v[2]};
    const float p[3]{
        ray.direction[1]*b[2]-ray.direction[2]*b[1],
        ray.direction[2]*b[0]-ray.direction[0]*b[2],
        ray.direction[0]*b[1]-ray.direction[1]*b[0]};
    const float det=a[0]*p[0]+a[1]*p[1]+a[2]*p[2];
    if (!std::isfinite(det) || std::fabs(det)<1.e-7f) return false;
    const float inv=1.f/det;
    const float t[3]{ray.origin[0]-v[0],ray.origin[1]-v[1],ray.origin[2]-v[2]};
    const float u=(t[0]*p[0]+t[1]*p[1]+t[2]*p[2])*inv;
    if (u<0.f || u>1.f) return false;
    const float q[3]{t[1]*a[2]-t[2]*a[1],
                     t[2]*a[0]-t[0]*a[2],
                     t[0]*a[1]-t[1]*a[0]};
    const float w=(ray.direction[0]*q[0]+ray.direction[1]*q[1]+
                   ray.direction[2]*q[2])*inv;
    if (w<0.f || u+w>1.f) return false;
    const float dist=(b[0]*q[0]+b[1]*q[1]+b[2]*q[2])*inv;
    if (!std::isfinite(dist) || dist<0.01f || dist>1000.f) return false;
    distance=dist;
    return true;
}

inline void retain_nearest_camera_triangles(const SceneRays& rays,
    SceneRayHits& nearest, const ogl_rtx_triangle& tri) {
    for (unsigned i=0;i<64;++i) {
        float distance=0.f;
        if (ray_hits_triangle(rays[i],tri,distance) &&
            distance<nearest[i].distance) {
            nearest[i].triangle=tri;
            nearest[i].distance=distance;
        }
    }
}
inline unsigned count_camera_hit_samples(const SceneRayHits& hits) {
    unsigned covered=0;
    for (const auto& h : hits) {
        if (std::isfinite(h.distance)) ++covered;
    }
    return covered;
}
inline unsigned append_camera_hit_triangles(const SceneRayHits& hits,
    std::vector<ogl_rtx_triangle>& out, std::size_t max_count) {
    unsigned anchors=0;
    for (const auto& h:hits) {
        if (out.size()>=max_count) break;
        if (std::isfinite(h.distance)) {
            // A single triangle may cover multiple ray samples; publish it
            // only once. Exact bitwise comparison is safe for the same mesh.
            bool seen=false;
            for (const auto& prev:out) {
                if (std::memcmp(prev.position,h.triangle.position,
                                sizeof(h.triangle.position))==0) {
                    seen=true; break;
                }
            }
            if (!seen) {out.push_back(h.triangle);++anchors;}
        }
    }
    return anchors;
}
} // namespace ogl
#endif
