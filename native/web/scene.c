/* Browser prototype: real native DAT/FigaTree/skin decoding, not a game loop.
 * WebGPU presentation is in renderer.js; no game assets are embedded here. */
#include "RenderBridge.h"
#include "melee_disc.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct { float p[3], n[3], color[4], uv[2]; } Vertex;
typedef struct {
    Vertex* vertices;
    uint32_t* indices;
    uint32_t count;
    const MeleeMaterialTexture* texture;
    MeleeGXPixelState pixel;
} Part;
static MeleeDisc* disc;
static MeleeModel* model;
static Part* parts;
static size_t part_count;
static char error[256];
static int initialized;
static unsigned omitted_layers;
static float duration;
static const struct { const char *code, *joint, *data; } fighters[] = {
    {"Mr", "PlyMario5K_Share_joint", "ftDataMario"},
    {"Fx", "PlyFox5K_Share_joint", "ftDataFox"},
    {"Pe", "PlyPeach5K_Share_joint", "ftDataPeach"},
};
const char* web_error(void) { return error; }
static int fail(const char* text) { snprintf(error, sizeof(error), "%s", text); return 0; }
void web_close_scene(void) {
    for (size_t i=0;i<part_count;i++) { free(parts[i].vertices); free(parts[i].indices); }
    free(parts); parts=NULL; part_count=0;
    melee_render_test_release(model); model=NULL;
}
int web_open_disc(const char* path) {
    web_close_scene(); melee_disc_close(disc); disc=NULL;
    if (!initialized) {
        if (!melee_render_test_init()) return fail("Unable to initialize the native model heap.");
        initialized=1;
    }
    disc=melee_disc_open(path);
    if (!disc) return fail("Choose a valid GameCube ISO or CISO disc image.");
    if (memcmp(melee_disc_id(disc),"GALE01",6)) {
        melee_disc_close(disc); disc=NULL;
        return fail("This prototype currently supports the USA Melee disc (GALE01).");
    }
    error[0]=0; return 1;
}
static int extract(const char* name,const char* path) {
    int32_t index=melee_disc_find(disc,0,name);
    const MeleeDiscEntry* entry=index<0?NULL:melee_disc_entry(disc,(uint32_t)index);
    if (!entry || entry->directory || !entry->length || entry->length>64*1024*1024)
        return fail("Required character asset is missing or too large.");
    uint8_t* bytes=malloc(entry->length);
    if (!bytes) return fail("Not enough memory for character assets.");
    int okay=melee_disc_read(disc,index,bytes,entry->length,0);
    FILE* file=okay?fopen(path,"wb"):NULL;
    okay=file && fwrite(bytes,1,entry->length,file)==entry->length;
    if (file && fclose(file)) okay=0;
    if (okay && !strcmp(path,"/motion.dat") && entry->length>=32) {
        uint32_t first=(uint32_t)bytes[0]<<24|(uint32_t)bytes[1]<<16|(uint32_t)bytes[2]<<8|bytes[3];
        MeleeArchive archive; const char* symbol; uint32_t root;
        if (first>entry->length || !melee_archive_open(&archive,bytes,first) ||
            !melee_archive_public(&archive,0,&symbol,&root) ||
            !melee_archive_f32(&archive,root+8,&duration) ||
            !isfinite(duration) || duration<=0 || duration>3600) okay=0;
    }
    free(bytes);
    return okay?1:fail("Unable to read the selected character from the disc.");
}
static int triangulate(Part* out,const MeleeModelPart* in) {
    if (!in->vertex_count || in->vertex_count>1000000) return 0;
    out->vertices=calloc(in->vertex_count,sizeof(Vertex));
    out->indices=malloc(in->vertex_count*3*sizeof(uint32_t));
    if (!out->vertices || !out->indices) return 0;
    for (size_t j=0;j<in->draw_count;j++) {
        const MeleeDraw* d=in->draws+j;
        if (d->first>in->vertex_count || d->count>in->vertex_count-d->first) return 0;
        for (size_t k=0;k<d->count;k++) {
            size_t a,b,c;
            switch (d->primitive) {
            case 0x90: if (k%3 || k+2>=d->count) continue; a=k;b=k+1;c=k+2;break;
            case 0x98: if(k<2)continue; a=k-2+(k&1);b=k-1-(k&1);c=k;break;
            case 0xa0: if(k<2)continue; a=0;b=k-1;c=k;break;
            case 0x80:
                if(k%4==0 && k+2<d->count){a=k;b=k+1;c=k+2;}
                else if(k%4==3){a=k-3;b=k-1;c=k;}else continue;
                break;
            default: return 0;
            }
            if(out->count+3>in->vertex_count*3)return 0;
            out->indices[out->count++]=d->first+a;
            out->indices[out->count++]=d->first+b;
            out->indices[out->count++]=d->first+c;
        }
    }
    return 1;
}
static void pack(void) {
    for(size_t i=0;i<part_count;i++) {
        const MeleeModelPart* p=melee_model_part(model,i);
        const MeleeMaterial* m=p->material;
        const MeleeMaterialTexture* t=parts[i].texture;
        for(size_t j=0;j<p->vertex_count;j++) {
            const MeleeVertex* src=p->vertices+j;Vertex* dst=parts[i].vertices+j;
            memcpy(dst->p,src->position,12);memcpy(dst->n,src->normal[0],12);
            for(unsigned c=0;c<4;c++) {
                dst->color[c]=m?(c==3?m->alpha:m->diffuse[c]/255.f):1;
                if(m && (m->render_mode&2))dst->color[c]*=src->color[0][c]/255.f;
            }
            if(t) {
                const float* uv=src->texcoord[t->source-4];
                for(unsigned c=0;c<2;c++)dst->uv[c]=t->matrix[c][0]*uv[0]+t->matrix[c][1]*uv[1]+t->matrix[c][2]+t->matrix[c][3];
            }
        }
    }
}
static int load_fighter(unsigned fighter) {
    if(!disc || fighter>=sizeof(fighters)/sizeof(fighters[0]))return fail("Choose a supported fighter and disc.");
    web_close_scene();omitted_layers=0;
    char path[32];
    snprintf(path,sizeof(path),"/Pl%sNr.dat",fighters[fighter].code);
    if(!extract(path,"/costume.dat"))return 0;
    snprintf(path,sizeof(path),"/Pl%sAJ.dat",fighters[fighter].code);
    if(!extract(path,"/motion.dat"))return 0;
    snprintf(path,sizeof(path),"/Pl%s.dat",fighters[fighter].code);
    if(!extract(path,"/fighter.dat"))return 0;
    model=melee_render_test_load("/costume.dat",fighters[fighter].joint,"/motion.dat",0);
    if(!model)return fail("The native model/animation decoder rejected this character.");
    if(!melee_render_test_fighter(model,"/fighter.dat",fighters[fighter].data,0)) {
        web_close_scene();return fail("Native fighter visibility setup failed.");
    }
    remove("/costume.dat");remove("/motion.dat");remove("/fighter.dat");
    part_count=melee_model_part_count(model);parts=calloc(part_count,sizeof(Part));
    if(!parts){part_count=0;web_close_scene();return fail("Not enough memory for geometry.");}
    for(size_t i=0;i<part_count;i++) {
        const MeleeModelPart* p=melee_model_part(model,i);const MeleeMaterial* m=p->material;
        if(!triangulate(parts+i,p)) {web_close_scene();return fail("Unsupported geometry in this prototype.");}
        if(!melee_gx_pixel_material(m?m->render_mode:0,m&&m->has_pixel_engine?m->pixel_engine:NULL,&parts[i].pixel)) {
            web_close_scene();return fail("Invalid material pixel state.");
        }
        if(m)for(size_t j=0;j<m->texture_count;j++) {
            const MeleeMaterialTexture* t=m->textures+j;
            if(!parts[i].texture && !(t->flags&15) && t->source>=4 && t->source<=11 && t->mip_count &&
               !(t->tev_active&0xc0000000) && ((t->flags>>16)&15)<=8 && ((t->flags>>20)&15)<=7)parts[i].texture=t;
            else omitted_layers++;
        }
    }
    pack();error[0]=0;return 1;
}
int web_load_fighter(unsigned fighter) {
    int result=load_fighter(fighter);
    remove("/costume.dat");remove("/motion.dat");remove("/fighter.dat");
    return result;
}
int web_step(float frame) {
    if(!model || !isfinite(frame) || frame<0)return fail("Invalid animation frame.");
    if(!melee_model_request(model,fmodf(frame,duration)) || !melee_model_step(model))return fail("Native animation update failed.");
    pack();return 1;
}
unsigned web_part_count(void){return part_count;}
unsigned web_omitted_layers(void){return omitted_layers;}
float web_duration(void){return duration;}
/* Fixed 32-bit wire layout, read by worker.js: 10 integers, 4 float settings,
 * then the 18 GX pixel register integers. No host struct offsets escape here. */
const uint32_t* web_part_info(unsigned index) {
    static uint32_t out[32];memset(out,0,sizeof(out));
    if(index>=part_count)return out;
    const MeleeModelPart* p=melee_model_part(model,index);
    const Part* part=parts+index;const MeleeMaterialTexture* t=part->texture;
    out[0]=(uintptr_t)part->vertices;out[1]=p->vertex_count;
    out[2]=(uintptr_t)part->indices;out[3]=part->count;out[9]=p->hidden;
    if(t){out[4]=(uintptr_t)t->mips[0].rgba;out[5]=t->mips[0].width;out[6]=t->mips[0].height;
        out[7]=t->wrap_s;out[8]=t->wrap_t;
        float settings[4]={(t->flags>>16)&15,(t->flags>>20)&15,t->blend,t->lod_bias};memcpy(out+10,settings,16);}
    memcpy(out+14,&part->pixel,sizeof(part->pixel));return out;
}
