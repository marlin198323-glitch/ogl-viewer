#include "ogl/ogl_cpu_buffer.h"
#include "ogl/ogl_fastcache_compare.h"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

// CPU-only microbenchmark: NOT disk latency, OpenSim loading or FPS.
int main() {
    constexpr std::size_t header=16, payload=1024, total=1040;
    constexpr int loops=180000;
    std::vector<unsigned char> previous(total), incoming(total);
    for(std::size_t i=0;i<total;++i)
        previous[i]=incoming[i]=static_cast<unsigned char>((i*53+19)%251);
    std::uint64_t matches=0,scalar_matches=0;
    const auto begin=std::chrono::steady_clock::now();
    std::size_t previous_index=header;
    for(int i=0;i<loops;++i) {
        incoming[previous_index]=previous[previous_index];
        const std::size_t changed_at=header+((i*97)%payload);
        previous_index=changed_at;
        incoming[changed_at]=static_cast<unsigned char>(
            previous[changed_at]^(i%3==0?1:0));
        matches+=ogl::same_fastcache_entry(previous.data(),incoming.data(),
                                             total,header,payload);
    }
    const auto middle=std::chrono::steady_clock::now();
    incoming[previous_index]=previous[previous_index];
    previous_index=header;
    for(int i=0;i<loops;++i) {
        incoming[previous_index]=previous[previous_index];
        const std::size_t changed_at=header+((i*97)%payload);
        previous_index=changed_at;
        incoming[changed_at]=static_cast<unsigned char>(
            previous[changed_at]^(i%3==0?1:0));
        scalar_matches+=
            ogl::count_changed_bytes_scalar(previous.data()+header,
                                            incoming.data()+header,payload)==0;
    }
    const auto end=std::chrono::steady_clock::now();
    std::uint64_t libc_matches=0;
    incoming[previous_index]=previous[previous_index];
    previous_index=header;
    for(int i=0;i<loops;++i) {
        incoming[previous_index]=previous[previous_index];
        const std::size_t changed_at=header+((i*97)%payload);
        previous_index=changed_at;
        incoming[changed_at]=static_cast<unsigned char>(
            previous[changed_at]^(i%3==0?1:0));
        libc_matches+=(std::memcmp(previous.data()+header,
                                  incoming.data()+header,payload)==0);
    }
    const auto libc_end=std::chrono::steady_clock::now();
    if(matches!=scalar_matches || matches!=libc_matches) return 2;
    const auto fast_us=std::chrono::duration_cast<std::chrono::microseconds>(
        middle-begin).count();
    const auto scalar_us=std::chrono::duration_cast<std::chrono::microseconds>(
        end-middle).count();
    const auto libc_us=std::chrono::duration_cast<std::chrono::microseconds>(
        libc_end-end).count();
    std::cout<<"OGL fastcache CPU-only microbenchmark backend="
             <<ogl::selected_buffer_backend()<<" entries="<<loops
             <<" payload="<<payload<<" bytes\n"
             <<"  simd_guard_us="<<fast_us
             <<" scalar_reference_us="<<scalar_us
             <<" libc_memcmp_us="<<libc_us
             <<" identical="<<matches<<"\n"
             <<"  WARNING: does not include extra disk read or skipped write, "
               "no FPS/loading speedup verified.\n";
    return 0;
}
