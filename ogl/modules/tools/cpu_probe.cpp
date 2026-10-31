#include "ogl/ogl_cpu_features.h"
#include <iostream>
int main() {
    const ogl::CpuFeatures f=ogl::detect_cpu_features();
    std::cout<<"OGL CPU: arch="<<ogl::arch_name(f.arch)
             <<" sse2="<<f.sse2<<" avx2="<<f.avx2
             <<" neon="<<f.neon<<" selected="<<ogl::path_name(ogl::select_cpu_path(f))
             <<"\n";
    return 0;
}
