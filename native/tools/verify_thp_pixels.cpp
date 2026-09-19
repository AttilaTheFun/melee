// Test-only independent decoder comparison; requires libjpeg-turbo.
#include "melee_thp.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>
#include <algorithm>
#include <jpeglib.h>

int main(int argc,char** argv) {
    assert(argc==2);
    std::ifstream file(argv[1],std::ios::binary);assert(file);
    std::vector<unsigned char> input((std::istreambuf_iterator<char>(file)),{});
    MeleeTHPInfo info{};assert(!melee_thp_info(input.data(),input.size(),&info));
    std::vector<unsigned char> planes[3]={std::vector<unsigned char>(info.y_bytes),
        std::vector<unsigned char>(info.uv_bytes),std::vector<unsigned char>(info.uv_bytes)};
    assert(!melee_thp_decode(input.data(),input.size(),planes[0].data(),planes[0].size(),
        planes[1].data(),planes[1].size(),planes[2].data(),planes[2].size()));
    size_t p=2;assert(input[0]==255&&input[1]==216);
    while(true) {
        assert(p+4<=input.size()&&input[p]==255);
        unsigned marker=input[p+1];size_t n=(input[p+2]<<8)|input[p+3];
        assert(n>=2&&n<=input.size()-p-2);p+=n+2;
        if(marker==218)break;
    }
    // Melee entropy bytes omit standard JPEG FF stuffing. Preserve the final EOI.
    assert(input.size()>=p+2&&input[input.size()-2]==255&&input.back()==217);
    std::vector<unsigned char> jpeg(input.begin(),input.begin()+p);
    for(size_t i=p;i<input.size()-2;++i){jpeg.push_back(input[i]);if(input[i]==255)jpeg.push_back(0);}
    jpeg.push_back(255);jpeg.push_back(217);
    jpeg_decompress_struct decoder{};jpeg_error_mgr error{};
    decoder.err=jpeg_std_error(&error);jpeg_create_decompress(&decoder);
    jpeg_mem_src(&decoder,jpeg.data(),jpeg.size());assert(jpeg_read_header(&decoder,TRUE)==JPEG_HEADER_OK);
    decoder.raw_data_out=TRUE;decoder.dct_method=JDCT_FLOAT;
    assert(jpeg_start_decompress(&decoder));
    assert(decoder.output_width==info.width&&decoder.output_height==info.height);
    std::vector<unsigned char> rows[3];std::vector<JSAMPROW> pointers[3];JSAMPARRAY arrays[3];
    for(int c=0;c<3;++c){size_t width=decoder.comp_info[c].width_in_blocks*8;
        int height=decoder.comp_info[c].v_samp_factor*8;rows[c].resize(width*height);
        for(int r=0;r<height;++r)pointers[c].push_back(rows[c].data()+r*width);
        arrays[c]=pointers[c].data();}
    unsigned maximum[3]={};unsigned long long total[3]={},counts[3]={};
    while(decoder.output_scanline<decoder.output_height){
        size_t top=decoder.output_scanline;assert(jpeg_read_raw_data(&decoder,arrays,16)==16);
        for(int c=0;c<3;++c){size_t width=c?(info.width+1)/2:info.width,height=c?(info.height+1)/2:info.height;
            size_t ytop=c?top/2:top;
            for(size_t r=0;r<pointers[c].size()&&ytop+r<height;++r)for(size_t x=0;x<width;++x){
                size_t y=ytop+r,offset=((y/4)*((width+7)/8)+x/8)*32+(y%4)*8+x%8;
                unsigned diff=std::abs(int(planes[c][offset])-int(pointers[c][r][x]));
                maximum[c]=std::max(maximum[c],diff);total[c]+=diff;++counts[c];}}
    }
    assert(jpeg_finish_decompress(&decoder));jpeg_destroy_decompress(&decoder);
    for(int c=0;c<3;++c)printf("%c max=%u mean=%.6f ","YUV"[c],maximum[c],double(total[c])/counts[c]);
    puts("");
    for(int c=0;c<3;++c)assert(maximum[c]<=1);
}
