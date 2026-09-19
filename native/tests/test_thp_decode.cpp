#include "melee_thp.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>
#include <algorithm>
#ifdef MELEE_TEST_STILL
extern "C" void test_game_still(const void*,size_t,int,int,const void*,const void*,const void*,size_t,size_t);
#endif

int main(int argc, char** argv) {
    assert(argc==1||argc==2);
    std::vector<unsigned char> input;
    if(argc==2) {
        std::ifstream file(argv[1],std::ios::binary);assert(file);
        input.assign(std::istreambuf_iterator<char>(file),{});
    } else {
        // One 16x16 neutral MCU: one-bit DC-zero and AC-EOB tables.
        input={255,216,255,219,0,67,0};input.insert(input.end(),64,1);
        auto append=[&](std::initializer_list<unsigned char> b){input.insert(input.end(),b);};
        append({255,192,0,17,8,0,16,0,16,3,1,0x22,0,2,0x11,0,3,0x11,0});
        for(unsigned char descriptor:{0,16}) {
            append({255,196,0,20,descriptor,1});input.insert(input.end(),15,0);input.push_back(0);
        }
        append({255,218,0,12,3,1,0,2,0,3,0,0,63,0,0,0,255,217});
    }
    MeleeTHPInfo info{};
    assert(!melee_thp_info(input.data(),input.size(),&info));
    std::vector<unsigned char> y(info.y_bytes,0xa5),u(info.uv_bytes,0xa5),v(info.uv_bytes,0xa5);
    assert(melee_thp_decode(input.data(),input.size(),y.data(),y.size()-1,u.data(),u.size(),v.data(),v.size()));
    assert(std::all_of(y.begin(),y.end(),[](auto x){return x==0xa5;}));
    for(size_t n=0;n<std::min(size_t(1024),input.size()-2);++n) {
        assert(melee_thp_decode(input.data(),n,y.data(),y.size(),u.data(),u.size(),v.data(),v.size()));
        assert(std::all_of(y.begin(),y.end(),[](auto x){return x==0xa5;}));
        assert(std::all_of(u.begin(),u.end(),[](auto x){return x==0xa5;}));
        assert(std::all_of(v.begin(),v.end(),[](auto x){return x==0xa5;}));
    }
    assert(!melee_thp_decode(input.data(),input.size(),y.data(),y.size(),u.data(),u.size(),v.data(),v.size()));
    unsigned long long sum=0;
    for(auto x:y)sum+=x;
    if(argc==1){assert(sum==256*128);for(auto x:u)assert(x==128);for(auto x:v)assert(x==128);}
#ifdef MELEE_TEST_STILL
    test_game_still(input.data(),input.size(),info.width,info.height,y.data(),u.data(),v.data(),y.size(),u.size());
#endif
    printf("THP %ux%u: bounded decode and truncated-input rollback passed; Y sum %llu\n",info.width,info.height,sum);
}
