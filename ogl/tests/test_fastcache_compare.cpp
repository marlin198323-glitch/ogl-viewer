#include "ogl/ogl_fastcache_compare.h"
#include <cassert>
#include <cstring>
#include <vector>
int main() {
    constexpr std::size_t header=16, data=1024, entry=1040;
    std::vector<unsigned char> a(entry,0x91), b(entry,0x91);
    // The data and all header bytes must match, not uninitialized padding.
    assert(ogl::same_fastcache_entry(a.data(),b.data(),entry,header,data));
    b[0]^=1;
    assert(!ogl::same_fastcache_entry(a.data(),b.data(),entry,header,data));
    b[0]^=1;
    b[header+100]^=1;
    assert(!ogl::same_fastcache_entry(a.data(),b.data(),entry,header,data));
    b[header+100]^=1;
    // Legacy image records can have a short valid payload and random tail.
    for(std::size_t i=header+40;i<entry;++i) b[i]^=0xff;
    assert(ogl::same_fastcache_entry(a.data(),b.data(),entry,header,40));
    assert(!ogl::same_fastcache_entry(a.data(),b.data(),entry,header,data));
    assert(!ogl::same_fastcache_entry(nullptr,b.data(),entry,header,data));
    assert(!ogl::same_fastcache_entry(a.data(),b.data(),entry,header,0));
    assert(!ogl::same_fastcache_entry(a.data(),b.data(),entry,0,data));
    assert(!ogl::same_fastcache_entry(a.data(),b.data(),entry,header,entry));
    assert(!ogl::same_fastcache_entry(a.data(),b.data(),entry,entry,1));
    return 0;
}
