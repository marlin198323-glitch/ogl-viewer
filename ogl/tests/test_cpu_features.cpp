#include "ogl/ogl_cpu_features.h"
#include <cassert>
int main() {
    using namespace ogl;
    assert(select_cpu_path(CpuFeatures{})==CpuPath::scalar);
    assert(select_cpu_path({CpuArch::x86_64,true,false,false})==CpuPath::sse2);
    assert(select_cpu_path({CpuArch::x86_64,true,true,false})==CpuPath::avx2);
    assert(select_cpu_path({CpuArch::x86,false,false,false})==CpuPath::scalar);
    assert(select_cpu_path({CpuArch::x86,true,true,false})==CpuPath::avx2);
    assert(select_cpu_path({CpuArch::arm64,false,false,true})==CpuPath::neon);
    assert(select_cpu_path({CpuArch::arm64,false,false,false})==CpuPath::scalar);
    // A bogus AVX2 flag on ARM must never select an x86 kernel.
    assert(select_cpu_path({CpuArch::arm64,false,true,true})==CpuPath::neon);
    assert(select_cpu_path({CpuArch::other,true,true,true})==CpuPath::scalar);
    const auto native=detect_cpu_features();
    const auto selected=select_cpu_path(native);
    if(native.arch==CpuArch::arm64) assert(selected==CpuPath::neon);
    if(native.arch==CpuArch::x86_64 || native.arch==CpuArch::x86) {
        assert(selected!=CpuPath::neon);
        if(selected==CpuPath::avx2) assert(native.avx2);
        if(selected==CpuPath::sse2) assert(native.sse2);
    }
    return 0;
}
