#include "ogl/ogl_cpu_buffer.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    using namespace ogl;
    const std::size_t lengths[]={0,1,2,7,15,16,17,31,32,33,63,64,65,
                                  127,128,257,1023,1024,1025,65536,65537};
    for (std::size_t n:lengths) {
        std::vector<unsigned char> a(n+5),b(n+5);
        for(std::size_t i=0;i<n+5;++i) {
            a[i]=static_cast<unsigned char>((i*97+19)%251);
            b[i]=a[i];
        }
        for(int mode=0;mode<5;++mode) {
            for(std::size_t i=0;i<n;++i) {
                b[i+1]=a[i+1];
                if(mode==1 && i%2==0) b[i+1]^=0x1;
                if(mode==2 && i%17==0) b[i+1]^=0x80;
                if(mode==3) b[i+1]^=0xff;
                if(mode==4 && i==n-1) b[i+1]^=0x1;
            }
            const auto reference=count_changed_bytes_scalar(a.data()+1,b.data()+1,n);
            assert(count_changed_bytes(a.data()+1,b.data()+1,n)==reference);
        }
    }
    assert(count_changed_bytes(nullptr,nullptr,15)==0);
    std::cout<<"OGL changed-byte kernel: "<<selected_buffer_backend()<<"\n";
    return 0;
}
