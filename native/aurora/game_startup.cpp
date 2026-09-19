// Original-game startup probe. No SDL video, window, surface or presentation.
// Bounded process lifetime; process exit is not a tested runtime shutdown.
#include "lib/internal.hpp"
#include "lib/webgpu/gpu.hpp"
#include "lib/webgpu/gpu_prof.hpp"
#include "lib/gfx/frame.hpp"
#include "lib/gfx/recording.hpp"
#include "lib/gx/fifo.hpp"
#include "lib/gx/texture.hpp"
#include "melee_reset.h"
#include "melee_vi.h"
#include "xfb.hpp"
#include <png.h>
#include <aurora/gfx.h>
#include <atomic>
#include <memory>
#include <vector>
extern "C" {
void ifAll_HideHUD(void);
void melee_startup_select_zelda(unsigned);
void melee_startup_select_link(unsigned,int);
void melee_startup_unlock_young_link(void);
int melee_startup_link_progress(int);
void melee_startup_select_iceclimbers(unsigned);
int melee_startup_iceclimbers_progress(void);
int melee_startup_iceclimbers_specials(unsigned);
void melee_startup_select_kirby(unsigned);
int melee_startup_kirby_progress(void);
int melee_startup_kirby_specials(unsigned);
int melee_startup_kirby_cache_reset_test(void);
int grBigBlue_NativeCarLaneTest(void);
int melee_startup_kirby_copy(unsigned,unsigned);
void melee_startup_kirby_copy_opponent(unsigned,unsigned);
void melee_startup_unlock_gamewatch(void);
void melee_startup_select_gamewatch(unsigned);
int melee_startup_gamewatch_progress(void);
int melee_startup_gamewatch_specials(unsigned);
void melee_startup_unlock_mewtwo(void);
void melee_startup_select_mewtwo(unsigned);
int melee_startup_mewtwo_progress(void);
int melee_startup_mewtwo_specials(unsigned);
void melee_startup_select_peach(unsigned);
int melee_startup_peach_progress(void);
int melee_startup_peach_specials(unsigned);
int melee_startup_peach_float(unsigned);
void melee_startup_select_samus(unsigned);
int melee_startup_samus_progress(void);
int melee_startup_samus_specials(unsigned);
int melee_startup_link_specials(unsigned,int,int);
int melee_startup_zelda_progress(unsigned,int);
int melee_startup_zelda_specials(unsigned);
void melee_startup_unlock_ganon(void);
void melee_startup_select_ganon(unsigned);
int melee_startup_ganon_progress(unsigned,int);
void melee_startup_unlock_doctor(void);
void melee_startup_select_doctor(unsigned);
int melee_startup_doctor_progress(unsigned,int);
#include "melee_dvd.h"
int melee_game_main(void);
unsigned mnStageSel_NativeFrames(void);
unsigned mnStageSel_NativeOnettGuidance(void);
unsigned mnStageSel_NativeFinalGuidance(void);
void melee_startup_unlock_final(void);
int melee_startup_final_progress(void);
int melee_startup_final_background_progress(unsigned);
unsigned mnStageSel_NativeBattleGuidance(void);
unsigned mnStageSel_NativeDreamlandGuidance(void);
void melee_startup_unlock_dreamland(void);
int melee_startup_dreamland_progress(void);
int melee_startup_story_hazards(unsigned frame);
int melee_startup_stadium_transform(unsigned frame,unsigned requested);
int melee_startup_stadium_active_form(void);
int melee_startup_stadium_form_end(unsigned frame);
int melee_startup_stadium_progress(void);
int melee_startup_story_progress(void);
int melee_startup_fountain_progress(void);
int melee_startup_fountain_platform_progress(unsigned);
unsigned mnStageSel_NativeStadiumGuidance(void);
unsigned mnStageSel_NativeGreatbayGuidance(void);
unsigned mnStageSel_NativeKongoGuidance(void);
unsigned mnStageSel_NativeJapesGuidance(void);
unsigned mnStageSel_NativePuraGuidance(void);
int melee_startup_pura_platforms(unsigned);
int melee_startup_pura_toon_test(void);
int melee_startup_classic_entry_progress(void);
int melee_startup_adventure_entry_progress(void);
void melee_startup_classic_navigate(unsigned);
int melee_startup_classic_layout_test(const char*);
int melee_startup_spawn_state_test(void);
int melee_startup_hand_laser_state_test(void);
int melee_startup_empty_demo_motion_test(void);
void fn_80185408(int,float,float,float,float);
int melee_startup_classic_match(unsigned);
int melee_startup_classic_round(unsigned);
int melee_startup_classic_bonus(unsigned);
int melee_startup_classic_after_bonus(unsigned);
int melee_startup_classic_fifth(unsigned);
int melee_startup_classic_trophy(unsigned);
int melee_startup_classic_team(unsigned);
int melee_startup_classic_race(unsigned);
int melee_startup_classic_gameover(unsigned);
int melee_startup_classic_continue(unsigned);
int melee_startup_classic_focus(unsigned,unsigned);
int melee_startup_player_mapping_test(void);
int melee_startup_ottosea_link_test(void);
int grIceMt_NativeContactFlagsTest(void);
int grHeal_NativeContactTest(void);
int ifStock_NativeAllstarRosterTest(void);
int grPushOn_NativeContactTest(void);
int melee_startup_hud_restart(unsigned, int);
int melee_startup_pura_progress(void);
void melee_startup_unlock_pura(void);
unsigned mnStageSel_NativeRCruiseGuidance(void);
int melee_startup_rcruise_traverse(unsigned);
int melee_startup_rcruise_progress(void);
unsigned mnStageSel_NativeKraidGuidance(void);
int melee_startup_kraid_hazards(unsigned);
int melee_startup_kraid_progress(void);
void melee_startup_unlock_kraid(void);
unsigned mnStageSel_NativeFlatzoneGuidance(void);
int melee_startup_flatzone_hazards(unsigned);
int melee_startup_flatzone_progress(void);
void melee_startup_unlock_flatzone(void);
unsigned mnStageSel_NativeIcemtGuidance(void);
int melee_startup_icemt_progress(void);
int melee_startup_icemt_scroll(unsigned);
int melee_startup_icemt_bear(unsigned);
unsigned mnStageSel_NativeFoursideGuidance(void);
int melee_startup_fourside_progress(void);
int melee_startup_fourside_hazards(unsigned);
void melee_startup_unlock_fourside(void);
unsigned mnStageSel_NativeBigBlueGuidance(void);
int melee_startup_bigblue_progress(void);
int melee_startup_bigblue_track(unsigned);
void melee_startup_unlock_bigblue(void);
unsigned mnStageSel_NativeMuteCityGuidance(void);
int melee_startup_mutecity_progress(void);
int melee_startup_mutecity_track(unsigned);
unsigned mnStageSel_NativeCorneriaGuidance(void);
int melee_startup_corneria_progress(void);
int melee_startup_corneria_hazards(unsigned);
unsigned mnStageSel_NativeVenomGuidance(void);
int melee_startup_venom_progress(void);
int melee_startup_venom_hazards(unsigned);
unsigned mnStageSel_NativeShrineGuidance(void);
int melee_startup_shrine_progress(void);
void melee_startup_results_rematch(unsigned);
unsigned mnStageSel_NativeGreensGuidance(void);
int melee_startup_greens_progress(void);
int melee_startup_greens_hazards(unsigned);
int melee_startup_greens_blocks(unsigned);
unsigned mnStageSel_NativeYorsterGuidance(void);
unsigned mnStageSel_NativeCastleGuidance(void);
unsigned mnStageSel_NativeBrinstarGuidance(void);
int melee_startup_yorster_progress(void);
int melee_startup_yorster_blocks(unsigned frame);
int melee_startup_castle_progress(void);
int melee_startup_castle_hazards(unsigned frame);
int melee_startup_brinstar_progress(void);
int melee_startup_brinstar_acid(unsigned frame);
int melee_startup_japes_progress(void);
int melee_startup_japes_hazards(unsigned frame);
int melee_startup_kongo_progress(void);
int melee_startup_greatbay_progress(void);
int melee_startup_greatbay_hazards(unsigned frame);
unsigned mnStageSel_NativeStoryGuidance(void);
unsigned mnStageSel_NativeFountainGuidance(void);
int melee_startup_dreamland_wind_progress(unsigned);
void melee_startup_unlock_stages(void);
int melee_startup_battle_progress(void);
int melee_startup_battle_background_progress(unsigned frame);
void lbDvd_NativeReportPreloads(void);
void GXWaitDrawDone(void);
void melee_startup_publish_confirm(int pressed);
int melee_startup_spawn_flower(void);
int melee_startup_spawn_goldeen(void);
int melee_startup_spawn_pokemon(unsigned selection);
int melee_startup_pokemon_progress(unsigned selection,unsigned frame);
void melee_startup_target_wobbuffet(unsigned frame);
int melee_startup_shorten_match(void);
int melee_startup_rematch_progress(unsigned frame);
int melee_startup_full_match_progress(unsigned frame);
int melee_startup_donkey_progress(unsigned frame,int require_specials);
int melee_startup_koopa_progress(unsigned frame,int require_specials);
int melee_startup_luigi_progress(unsigned frame,int require_specials);
void melee_startup_unlock_luigi(void);
void melee_startup_unlock_roy(void);
void melee_startup_unlock_pichu(void);
void melee_startup_unlock_purin(void);
int melee_startup_purin_progress(void);
int melee_startup_yoshi_progress(void);
void melee_startup_select_yoshi(unsigned);
int melee_startup_yoshi_specials(unsigned);
int melee_startup_yoshi_capture(unsigned);
int melee_startup_purin_specials(unsigned);
int melee_startup_purin_hat_progress(void);
int melee_startup_pichu_progress(void);
int melee_startup_event_preview(unsigned);
int melee_startup_pikachu_progress(void);
int melee_startup_pikachu_specials(unsigned,int);
int melee_startup_roy_progress(void);
void melee_startup_unlock_marth(void);
int melee_startup_marth_progress(void);
int melee_startup_marth_specials(unsigned);
int melee_startup_roy_specials(unsigned);
int melee_startup_captain_progress(unsigned frame,int require_specials);
int melee_startup_mario_progress(unsigned frame,int require_specials);
void melee_startup_publish_start(int pressed);
void melee_startup_publish_down(int pressed);
void melee_startup_publish_stick(int x,int y);
void melee_startup_publish_opponent(int x,int y,int confirm);
void melee_startup_report_fighters(unsigned frame);
void melee_startup_audio_start(void);
void melee_startup_audio_finish(void);
void melee_startup_publish_combat(unsigned slot,int x,int y,unsigned buttons);
int melee_startup_match_coins(unsigned frame);
}
#include <filesystem>
#include <thread>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <unistd.h>

namespace {
unsigned frames;
const char* capture_path;
bool confirm_script;
bool title_script;
bool menu_script;
bool versus_script;
bool select_script;
bool stage_script;
bool match_script;
bool combat_script;
bool pause_script;
bool finish_script;
bool flower_script;
bool goldeen_script;
unsigned pokemon_script;
bool transition_script;
bool mario_script;
bool doctor_script;
bool captain_script;
bool ganon_script;
bool zelda_script;
bool link_script;
bool iceclimbers_script;
bool iceclimbers_specials_script;
unsigned kirby_copy_variant;
bool kirby_copy_script;
bool kirby_script;
bool kirby_specials_script;
bool gamewatch_script;
bool gamewatch_specials_script;
bool mewtwo_script;
bool mewtwo_specials_script;
bool peach_script;
bool peach_specials_script;
bool peach_float_script;
bool samus_script;
bool samus_specials_script;
bool young_link_script;
bool link_specials_script;
bool link_extras_script;
bool zelda_specials_script;
bool donkey_script;
bool koopa_script;
bool roy_script;
bool event_preview_script;
bool pikachu_script;
bool pikachu_specials_script;
bool pichu_script;
bool purin_script;
bool yoshi_script;
bool yoshi_specials_script;
bool yoshi_capture_script;
bool purin_specials_script;
bool purin_hat_script;
bool roy_specials_script;
bool marth_specials_script;
bool marth_script;
bool luigi_script;
bool final_background_script;
bool final_script;
bool battle_script;
bool dreamland_script;
bool story_hazards_script;
unsigned stadium_requested_form;
bool stadium_transform_script;
bool stadium_form_rematch_script;
bool stadium_script;
bool greatbay_script;
bool kongo_script;
bool japes_script;
bool greens_blocks_script;
bool greens_hazards_script;
bool flatzone_hazards_script;
bool kraid_hazards_script;
bool rcruise_traverse_script;
bool pura_platforms_script;
bool pura_script;
bool rcruise_script;
bool kraid_script;
bool flatzone_script;
bool icemt_script;
bool icemt_scroll_script;
bool icemt_bear_script;
bool fourside_script;
bool fourside_hazards_script;
bool bigblue_script;
bool bigblue_track_script;
bool mutecity_script;
bool mutecity_track_script;
bool corneria_script;
bool corneria_hazards_script;
bool venom_script;
bool venom_hazards_script;
bool shrine_script;
bool greens_script;
bool yorster_script;
bool yorster_blocks_script;
bool castle_script;
bool castle_hazards_script;
bool brinstar_script;
bool brinstar_acid_script;
bool japes_hazards_script;
bool greatbay_hazards_script;
bool story_script;
bool fountain_script;
bool fountain_platform_script;
bool dreamland_wind_script;
bool battle_background_script;
bool rematch_script;
bool classic_entry_script;
bool classic_match_script;
bool classic_round_script;
bool classic_bonus_script;
bool classic_after_bonus_script;
bool classic_fifth_script;
bool classic_trophy_script;
bool classic_team_script;
bool classic_race_script;
bool classic_gameover_script;
bool classic_continue_script;
unsigned classic_focus_script;
bool intro_mask_script;
bool hud_restart_script;
bool hud_stamina_script;
bool audio_script;
bool debug_stop;
unsigned captured_match_frames;
bool capture_frame(bool reflection=false, int copy_index=-1) {
    using namespace aurora;
    const auto presentation=melee_vi_presentation();
    auto source=melee::xfb::image(presentation.framebuffer);
    if(!source){std::fputs("No latched framebuffer image to capture\n",stderr);return false;}
    unsigned width=640,height=480;
    if(reflection){
        unsigned matches=0;
        for(const auto& [key,copy]:gx::g_gxState.copyTextureCache)
            if(key.width==80&&key.height==60){source=copy.handle;matches++;}
        if(matches!=1){std::fprintf(stderr,"Expected one reflection texture, found %u\n",matches);return false;}
        width=80;height=60;
    }
    if(copy_index>=0){
        int index=0;bool found=false;
        for(const auto& [key,copy]:gx::g_gxState.copyTextureCache){
            if(index++!=copy_index)continue;
            source=copy.handle;width=key.width;height=key.height;found=true;
            std::fprintf(stderr,"GPU copy capture index=%d destination=%p size=%ux%u format=%u\n",copy_index,key.dest,width,height,unsigned(source->format));break;
        }
        if(!found||!source)return false;
    }
    const unsigned stride=(width*4+255)&~255u,size=stride*height;
    wgpu::BufferDescriptor descriptor{.label="Original startup frame readback",
        .usage=wgpu::BufferUsage::CopyDst|wgpu::BufferUsage::MapRead,.size=size};
    auto buffer=webgpu::g_device.CreateBuffer(&descriptor);
    auto encoder=webgpu::g_device.CreateCommandEncoder();
    const wgpu::TexelCopyTextureInfo texture{.texture=source->texture};
    const wgpu::TexelCopyBufferInfo destination{.layout={.bytesPerRow=stride,.rowsPerImage=height},.buffer=buffer};
    const wgpu::Extent3D extent{width,height,1};encoder.CopyTextureToBuffer(&texture,&destination,&extent);
    auto command=encoder.Finish();webgpu::g_queue.Submit(1,&command);
    auto mapped=std::make_shared<std::atomic<bool>>(false);
    auto future=buffer.MapAsync(wgpu::MapMode::Read,0,size,wgpu::CallbackMode::WaitAnyOnly,
        [mapped](wgpu::MapAsyncStatus status,wgpu::StringView){mapped->store(status==wgpu::MapAsyncStatus::Success);});
    if(webgpu::g_instance.WaitAny(future,10000000000)!=wgpu::WaitStatus::Success||!mapped->load())return false;
    // XFB scanout is opaque. Its backing texture's alpha is not display
    // transparency (retail rendering can leave it zero across the frame).
    // Export only RGB so image viewers reproduce the displayed colors.
    const auto* rgba=static_cast<const unsigned char*>(buffer.GetConstMappedRange());
    std::vector<unsigned char> rgb(width*height*3);
    for(unsigned i=0;i<width*height;++i)
        for(unsigned channel=0;channel<3;++channel)rgb[i*3+channel]=rgba[(i/width)*stride+(i%width)*4+channel];
    png_image image{};image.version=PNG_IMAGE_VERSION;image.width=width;image.height=height;image.format=PNG_FORMAT_RGB;
    const std::string output=copy_index>=0?std::string(capture_path)+".copy-"+std::to_string(copy_index)+".png":reflection?std::string(capture_path)+".reflection.png":capture_path;
    bool okay=png_image_write_to_file(&image,output.c_str(),0,rgb.data(),width*3,nullptr)!=0;
    png_image_free(&image);buffer.Unmap();
    std::fprintf(stderr,"Original framebuffer capture: black=%d retrace=%u output=%s\n",presentation.black,presentation.retrace,output.c_str());
    if(okay&&!reflection&&copy_index<0&&frames>=2800&&std::getenv("MELEE_STARTUP_REFLECTION"))okay=capture_frame(true);
    if(okay&&!reflection&&copy_index<0&&frames>=2800&&std::getenv("MELEE_STARTUP_GPU_COPIES"))
        for(unsigned i=0;i<gx::g_gxState.copyTextureCache.size();i++)if(!capture_frame(false,i))okay=false;
    return okay;
}

void submit_frame() {
    using namespace aurora;
    if(intro_mask_script&&frames>=60&&frames<120){
        fn_80185408(0,0,480,0,640);
        if(frames==119)std::fputs("Native intro mask: 60 projection/vertex submissions completed\n",stderr);
    }
    gx::fifo::drain();
    if(pause_script&&frames==2799){
        if(const char* path=std::getenv("MELEE_STARTUP_GX_DUMP")){
            const auto& state=gx::g_gxState;
            if(FILE* file=std::fopen(path,"w")){
                std::fprintf(file,"blend mode=%u src=%u dst=%u stages=%u\n",unsigned(state.blendMode),unsigned(state.blendFacSrc),unsigned(state.blendFacDst),unsigned(state.numTevStages));
                for(unsigned i=0;i<state.numTevStages;i++){
                    const auto& stage=state.tevStages[i];
                    std::fprintf(file,"stage=%u alpha=%u,%u,%u,%u op=%u clamp=%u tex=%u swap=%u\n",i,unsigned(stage.alphaPass.a),unsigned(stage.alphaPass.b),unsigned(stage.alphaPass.c),unsigned(stage.alphaPass.d),unsigned(stage.alphaOp.op),unsigned(stage.alphaOp.clamp),unsigned(stage.texMapId),unsigned(stage.tevSwapTex));
                }
                std::fclose(file);
            }
        }
    }
    gx::fifo::end_frame();gx::texture::end_frame();gfx::finish();
    gfx::end_frame([](wgpu::CommandEncoder& encoder,std::vector<gfx::AfterSubmitCallback> callbacks){
        webgpu::gpu_prof::frame_end(encoder);
        auto commands=encoder.Finish();webgpu::g_queue.Submit(1,&commands);
        webgpu::gpu_prof::after_submit();for(auto& callback:callbacks)callback();gfx::after_submit();
    });
    GXWaitDrawDone();
    if(audio_script&&frames==0)melee_startup_audio_start();
    if(frames>=1560){const auto* stats=aurora_get_stats();
        std::fprintf(stderr,"Match frame storage bytes: %u\n",stats->lastStorageSize);
        if(match_script&&capture_path&&captured_match_frames<3&&stats->lastStorageSize>8388608){
            if(capture_frame())++captured_match_frames;
            std::fprintf(stderr,"Captured high-storage presentation checkpoint %u of 3\n",captured_match_frames);
        }
    }

    std::fprintf(stderr,"Original game GPU submission %u completed\n",++frames);
    if(confirm_script && (frames==180||frames==186||frames==300||frames==306||(title_script&&(frames==450||frames==456)))){
        melee_startup_publish_confirm(frames==180||frames==300||frames==450);
        std::fprintf(stderr,"Scripted controller A %s at submission %u\n",(frames==180||frames==300||frames==450)?"pressed":"released",frames);
    }
    if(debug_stop&&frames==600){
        std::fprintf(stderr,"Debugger attach point: pid=%ld\n",(long)getpid());
        std::raise(SIGSTOP);
    }
    if(menu_script&&(frames==600||frames==606)){
        melee_startup_publish_start(frames==600);
        std::fprintf(stderr,"Scripted controller Start %s at submission %u\n",frames==600?"pressed":"released",frames);
    }
    if(versus_script&&(frames==720||frames==726)){
        melee_startup_publish_down(frames==720);
        std::fprintf(stderr,"Scripted controller Down %s at submission %u\n",frames==720?"pressed":"released",frames);
    }
    if(versus_script&&(frames==780||frames==786||frames==900||frames==906)){
        melee_startup_publish_confirm(frames==780||frames==900);
        std::fprintf(stderr,"Scripted controller A %s at submission %u\n",(frames==780||frames==900)?"pressed":"released",frames);
    }
    if(classic_entry_script){
        melee_startup_classic_navigate(frames);
        if(frames==780||frames==786||frames==900||frames==906||frames==1020||frames==1026)
            melee_startup_publish_confirm(frames==780||frames==900||frames==1020);
        if(classic_focus_script>=9&&(frames==960||frames==966))melee_startup_publish_down(frames==960);
        if(frames==1800&&!(classic_focus_script>=9?melee_startup_adventure_entry_progress():melee_startup_classic_entry_progress())){
            if(capture_path)capture_frame();std::_Exit(6);
        }
    }
    if(classic_match_script){
        if(frames==2400&&capture_path)capture_frame();
        int complete=melee_startup_classic_match(frames);
        if(frames==3800&&!classic_gameover_script&&!classic_focus_script&&!complete){if(capture_path)capture_frame();std::_Exit(6);}
    }
    if(classic_round_script){int complete=melee_startup_classic_round(frames);if(frames==6500&&!complete){if(capture_path)capture_frame();std::_Exit(6);}}
    if(hud_restart_script&&!melee_startup_hud_restart(frames,hud_stamina_script)){if(capture_path)capture_frame();std::_Exit(6);}
    if(combat_script&&std::getenv("MELEE_TEST_MATCH_COINS")){
        int complete=melee_startup_match_coins(frames);
        if(frames==2400&&!complete){std::fputs("Match coin tiers incomplete\n",stderr);std::_Exit(6);}
    }
    if(classic_focus_script){int complete=melee_startup_classic_focus(frames,classic_focus_script);if((classic_focus_script==8||classic_focus_script==18)&&complete){if(capture_path&&!capture_frame())std::_Exit(5);std::fputs(classic_focus_script==8?(std::getenv("MELEE_TEST_ALLSTAR_CAMPAIGN")?"All-Star campaign through credits entry passed\n":std::getenv("MELEE_TEST_ALLSTAR")?"All-Star opening fight and rest entry passed\n":"Continuous Classic campaign passed\n"):(std::getenv("MELEE_TEST_ADVENTURE_CHECKPOINT_SCENE")&&std::atoi(std::getenv("MELEE_TEST_ADVENTURE_CHECKPOINT_SCENE"))>=64)?"Adventure late checkpoint transition passed\n":std::getenv("MELEE_TEST_ADVENTURE_ONETT_ENTRY")?"Adventure F-Zero through Onett entry passed\n":std::getenv("MELEE_TEST_ADVENTURE_FZERO")?"Adventure F-Zero progression passed\n":std::getenv("MELEE_TEST_ADVENTURE_STARFOX")?"Adventure Star Fox progression passed\n":std::getenv("MELEE_TEST_ADVENTURE_KIRBY")?"Adventure Kirby progression passed\n":std::getenv("MELEE_TEST_ESCAPE_COMPLETE")?"Brinstar escape and explosion transition passed\n":"Brinstar escape entry passed\n",stderr);std::_Exit(0);}if((frames==3500||(classic_focus_script>=4&&(frames==4250||frames==5500)))&&capture_path){const char* original=capture_path;std::string checkpoint=std::string(original)+(frames==4250?".ending.png":frames==5500?".credits.png":".round.png");capture_path=checkpoint.c_str();capture_frame();capture_path=original;}if(frames==(classic_focus_script==18?(std::getenv("MELEE_TEST_ADVENTURE_KIRBY")?23000u:19000u):classic_focus_script==17?18000u:classic_focus_script==16?16500u:classic_focus_script==15?14500u:classic_focus_script==14?12500u:classic_focus_script==13?8500u:classic_focus_script==12?7500u:classic_focus_script==10?9000u:classic_focus_script==8?(std::getenv("MELEE_TEST_ALLSTAR_CAMPAIGN")?36000u:24000u):classic_focus_script==7?11500u:classic_focus_script==6?8000u:classic_focus_script==5?10500u:5000u)&&!complete){if(capture_path)capture_frame();std::_Exit(6);}}
    if(classic_continue_script){int complete=melee_startup_classic_continue(frames);if(frames==9000&&!complete){if(capture_path)capture_frame();std::_Exit(6);}}
    if(classic_gameover_script){int complete=melee_startup_classic_gameover(frames);if(frames==4500&&!complete){if(capture_path)capture_frame();std::_Exit(6);}}
    if(classic_race_script){int complete=melee_startup_classic_race(frames);if(frames==24000&&!complete){if(capture_path)capture_frame();std::_Exit(6);}}
    if(classic_team_script){int complete=melee_startup_classic_team(frames);if(frames==21000&&!complete){if(capture_path)capture_frame();std::_Exit(6);}}
    if(classic_trophy_script){int complete=melee_startup_classic_trophy(frames);if(frames==18000&&!complete){if(capture_path)capture_frame();std::_Exit(6);}}
    if(classic_fifth_script){int complete=melee_startup_classic_fifth(frames);if(frames==15000&&!complete){if(capture_path)capture_frame();std::_Exit(6);}}
    if(classic_after_bonus_script){int complete=melee_startup_classic_after_bonus(frames);if(frames==12000&&!complete){if(capture_path)capture_frame();std::_Exit(6);}}
    if(classic_bonus_script){int complete=melee_startup_classic_bonus(frames);if(frames==9000&&!complete){if(capture_path)capture_frame();std::_Exit(6);}}
    if(event_preview_script){int complete=melee_startup_event_preview(frames);if(frames==4500&&!complete){std::fputs("Event preview coverage incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(battle_background_script){int complete=melee_startup_battle_background_progress(frames);if(frames==6500&&!complete){std::fputs("Battlefield background did not complete a transition\n",stderr);std::_Exit(6);}}
    if(final_script&&finish_script)melee_startup_final_background_progress(frames);
    if(final_background_script){int complete=melee_startup_final_background_progress(frames);if(frames==6500&&!complete){std::fputs("Final Destination background transition incomplete\n",stderr);std::_Exit(6);}}
    if(final_script&&frames==800)melee_startup_unlock_final();
    if(final_script&&frames==(final_background_script?6500u:rematch_script?6000u:2800u)&&!melee_startup_final_progress()){std::fputs("Final Destination did not reach live match\n",stderr);std::_Exit(6);}
    if(dreamland_wind_script){int complete=melee_startup_dreamland_wind_progress(frames);if(frames==6500&&!complete){std::fputs("Dream Land wind cycle incomplete\n",stderr);std::_Exit(6);}}
    if(fountain_platform_script){int complete=melee_startup_fountain_platform_progress(frames);if(frames==6500&&!complete){std::fputs("Fountain platforms did not move\n",stderr);std::_Exit(6);}}
    if(story_hazards_script){int complete=melee_startup_story_hazards(frames);if(frames==6500&&!complete){std::fputs("Story hazards coverage incomplete\n",stderr);std::_Exit(6);}}
    if((stadium_transform_script||stadium_form_rematch_script)&&frames==6200&&capture_path){
        const char* original=capture_path;const std::string form_path=std::string(original)+".form.png";
        capture_path=form_path.c_str();bool okay=capture_frame();capture_path=original;if(!okay)std::_Exit(5);
    }
    if(stadium_form_rematch_script){
        melee_startup_stadium_transform(frames,6);
        int ended=melee_startup_stadium_form_end(frames);
        if(frames==10400&&!ended){std::fputs("Stadium did not end while rock form was active\n",stderr);std::_Exit(6);}
        if(frames==6200&&melee_startup_stadium_active_form()!=6){std::fputs("Stadium rematch did not reach rock form\n",stderr);std::_Exit(6);}
    }
    if(stadium_transform_script){int complete=melee_startup_stadium_transform(frames,stadium_requested_form);if(frames==8500&&!complete){std::fputs("Stadium transform cycle incomplete\n",stderr);std::_Exit(6);}}
    if(greatbay_hazards_script){int complete=melee_startup_greatbay_hazards(frames);if(frames==7800&&!complete){std::fputs("Great Bay hazard cycle incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(japes_hazards_script){int complete=melee_startup_japes_hazards(frames);if(frames==6500&&!complete){std::fputs("Japes hazard coverage incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(brinstar_acid_script){int complete=melee_startup_brinstar_acid(frames);if(frames==6500&&!complete){std::fputs("Brinstar acid cycle incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(castle_hazards_script){
        int status=melee_startup_castle_hazards(frames);static bool captured=false;
        if(status==2&&!captured&&capture_path){
            const char* original=capture_path;const std::string path=std::string(original)+".explosion.png";
            capture_path=path.c_str();bool okay=capture_frame();capture_path=original;if(!okay)std::_Exit(5);captured=true;
        }
        if(frames==8200&&!(status&1)){std::fputs("Castle Banzai Bill cycle incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}
    }
    if(mutecity_track_script){int complete=melee_startup_mutecity_track(frames);if(frames==7800&&!complete){std::fputs("Mute City track coverage incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(bigblue_track_script){int complete=melee_startup_bigblue_track(frames);if(frames==7800&&!complete){std::fputs("Big Blue track coverage incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(fourside_hazards_script){int complete=melee_startup_fourside_hazards(frames);if(frames==7800&&!complete){std::fputs("Fourside hazard coverage incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(rcruise_traverse_script){int complete=melee_startup_rcruise_traverse(frames);if(frames==7800&&!complete){std::fputs("Rainbow Cruise traversal incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(rcruise_script&&frames==(rcruise_traverse_script?7800u:rematch_script?6000u:2800u)&&!melee_startup_rcruise_progress()){std::fputs("Rainbow Cruise did not reach live match\n",stderr);std::_Exit(6);}
    if(pura_script&&frames==800)melee_startup_unlock_pura();
    if(pura_platforms_script){int complete=melee_startup_pura_platforms(frames);if(frames==7800&&!complete){std::fputs("Poke Floats platform coverage incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(pura_script&&frames==(pura_platforms_script?7800u:2800u)&&!melee_startup_pura_progress()){std::fputs("Poke Floats did not reach live match\n",stderr);std::_Exit(6);}
    if(kraid_script&&frames==800)melee_startup_unlock_kraid();
    if(kraid_hazards_script){int complete=melee_startup_kraid_hazards(frames);if(frames==7800&&!complete){std::fputs("Kraid hazard coverage incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(kraid_script&&frames==(kraid_hazards_script?7800u:rematch_script?6000u:2800u)&&!melee_startup_kraid_progress()){std::fputs("Brinstar Depths did not reach live match\n",stderr);std::_Exit(6);}
    if(flatzone_script&&frames==800)melee_startup_unlock_flatzone();
    if(flatzone_hazards_script){int complete=melee_startup_flatzone_hazards(frames);if(frames==7800&&!complete){std::fputs("Flat Zone hazard coverage incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(flatzone_script&&frames==(flatzone_hazards_script?7800u:rematch_script?6000u:2800u)&&!melee_startup_flatzone_progress()){std::fputs("Flat Zone did not reach live match\n",stderr);std::_Exit(6);}
    if(icemt_bear_script&&frames==2000&&capture_path){
        const char* original=capture_path;std::string bear_capture=std::string(original)+".bear.png";
        capture_path=bear_capture.c_str();if(!capture_frame())std::_Exit(5);capture_path=original;
    }
    if(icemt_bear_script){int complete=melee_startup_icemt_bear(frames);if(frames==(classic_focus_script==18?(std::getenv("MELEE_TEST_ADVENTURE_KIRBY")?23000u:19000u):classic_focus_script==17?18000u:classic_focus_script==16?16500u:classic_focus_script==15?14500u:classic_focus_script==14?12500u:classic_focus_script==13?8500u:classic_focus_script==12?7500u:classic_focus_script==10?9000u:classic_focus_script==8?(std::getenv("MELEE_TEST_ALLSTAR_CAMPAIGN")?36000u:24000u):classic_focus_script==7?11500u:classic_focus_script==6?8000u:classic_focus_script==5?10500u:5000u)&&!complete){std::fputs("Polar Bear coverage incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(icemt_scroll_script){int complete=melee_startup_icemt_scroll(frames);if(frames==7800&&!complete){std::fputs("Icicle scrolling coverage incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(icemt_script&&rematch_script&&frames<=6200){
        int complete=melee_startup_icemt_scroll(frames);
        if(frames==6200&&!complete){std::fputs("Icicle rematch requires prior segment replacement\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}
    }
    if(icemt_script&&frames==(icemt_bear_script?5000u:icemt_scroll_script?7800u:rematch_script?10000u:2800u)&&!melee_startup_icemt_progress()){std::fputs("Icicle Mountain did not reach live match\n",stderr);std::_Exit(6);}
    if(fourside_script&&frames==(fourside_hazards_script?7800u:rematch_script?6000u:2800u)&&!melee_startup_fourside_progress()){std::fputs("Fourside did not reach live match\n",stderr);std::_Exit(6);}
    if(bigblue_script&&frames==(bigblue_track_script?7800u:rematch_script?6000u:2800u)&&!melee_startup_bigblue_progress()){std::fputs("Big Blue did not reach live match\n",stderr);std::_Exit(6);}
    if(mutecity_script&&frames==(mutecity_track_script?7800u:rematch_script?6000u:2800u)&&!melee_startup_mutecity_progress()){std::fputs("Mute City did not reach live match\n",stderr);std::_Exit(6);}
    if(corneria_hazards_script){int complete=melee_startup_corneria_hazards(frames);if(frames==7800&&!complete){std::fputs("Corneria hazard coverage incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(corneria_script&&frames==(corneria_hazards_script?7800u:rematch_script?6000u:2800u)&&!melee_startup_corneria_progress()){std::fputs("Corneria did not reach live match\n",stderr);std::_Exit(6);}
    if(venom_hazards_script){int complete=melee_startup_venom_hazards(frames);if(frames==7800&&!complete){std::fputs("Venom hazard coverage incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(venom_script&&frames==(venom_hazards_script?7800u:rematch_script?6000u:2800u)&&!melee_startup_venom_progress()){std::fputs("Venom did not reach live match\n",stderr);std::_Exit(6);}
    if(shrine_script&&frames==(rematch_script?11000u:2800u)&&!melee_startup_shrine_progress()){std::fputs("Temple did not reach live match\n",stderr);std::_Exit(6);}
    if(greens_blocks_script){int complete=melee_startup_greens_blocks(frames);if(frames==6500&&!complete){std::fputs("Green Greens block coverage incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(greens_hazards_script){int complete=melee_startup_greens_hazards(frames);if(frames==7800&&!complete){std::fputs("Green Greens hazard coverage incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(greens_script&&frames==(greens_blocks_script?6500u:greens_hazards_script?7800u:rematch_script?8000u:2800u)&&!melee_startup_greens_progress()){std::fputs("Green Greens did not reach live match\n",stderr);std::_Exit(6);}
    if(yorster_blocks_script){int complete=melee_startup_yorster_blocks(frames);if(frames==6500&&!complete){std::fputs("Yoshi Island block cycle incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(yorster_script&&frames==(yorster_blocks_script?6500u:rematch_script?8000u:2800u)&&!melee_startup_yorster_progress()){std::fputs("Yoshi Island did not reach live match\n",stderr);std::_Exit(6);}
    if(castle_script&&frames==(castle_hazards_script?8200u:rematch_script?6000u:2800u)&&!melee_startup_castle_progress()){std::fputs("Castle did not reach live match\n",stderr);std::_Exit(6);}
    if(brinstar_script&&frames==(brinstar_acid_script?6500u:rematch_script?6000u:2800u)&&!melee_startup_brinstar_progress()){std::fputs("Brinstar did not reach live match\n",stderr);std::_Exit(6);}
    if(japes_script&&frames==(japes_hazards_script?6500u:rematch_script?6000u:2800u)&&!melee_startup_japes_progress()){std::fputs("Jungle Japes did not reach live match\n",stderr);std::_Exit(6);}
    if(kongo_script&&frames==(rematch_script?6000u:2800u)&&!melee_startup_kongo_progress()){std::fputs("Kongo Jungle did not reach live match\n",stderr);std::_Exit(6);}
    if(greatbay_script&&frames==(greatbay_hazards_script?7800u:rematch_script?6000u:2800u)&&!melee_startup_greatbay_progress()){std::fputs("Great Bay did not reach live match\n",stderr);std::_Exit(6);}
    if(stadium_script&&frames==(stadium_transform_script?8500u:stadium_form_rematch_script?10400u:rematch_script?6000u:2800u)&&!melee_startup_stadium_progress()){std::fputs("Stadium did not reach live match\n",stderr);std::_Exit(6);}
    if(story_script&&frames==(story_hazards_script?6500u:rematch_script?6000u:2800u)&&!melee_startup_story_progress()){std::fputs("Yoshi Story did not reach live match\n",stderr);std::_Exit(6);}
    if((fountain_script||shrine_script)&&frames==1800&&std::getenv("MELEE_STARTUP_NO_HUD"))ifAll_HideHUD();
    if(fountain_script&&frames==(fountain_platform_script?6500u:rematch_script?8000u:2800u)&&!melee_startup_fountain_progress()){std::fputs("Fountain did not reach live match\n",stderr);std::_Exit(6);}
    if(dreamland_script&&frames==800)melee_startup_unlock_dreamland();
    if(dreamland_script&&frames==(dreamland_wind_script?6500u:rematch_script?6000u:2800u)&&!melee_startup_dreamland_progress()){std::fputs("Dream Land did not reach live match\n",stderr);std::_Exit(6);}
    if(fourside_script&&frames==800)melee_startup_unlock_fourside();
    if(bigblue_script&&frames==800)melee_startup_unlock_bigblue();
    if(battle_script&&frames==800)melee_startup_unlock_stages();
    if(battle_script&&frames==(battle_background_script?6500u:rematch_script?6000u:2800u)&&!melee_startup_battle_progress()){std::fputs("Battlefield did not reach live match\n",stderr);std::_Exit(6);}
    if(pikachu_specials_script){int complete=melee_startup_pikachu_specials(frames,pichu_script);if(frames==(classic_focus_script==18?(std::getenv("MELEE_TEST_ADVENTURE_KIRBY")?23000u:19000u):classic_focus_script==17?18000u:classic_focus_script==16?16500u:classic_focus_script==15?14500u:classic_focus_script==14?12500u:classic_focus_script==13?8500u:classic_focus_script==12?7500u:classic_focus_script==10?9000u:classic_focus_script==8?(std::getenv("MELEE_TEST_ALLSTAR_CAMPAIGN")?36000u:24000u):classic_focus_script==7?11500u:classic_focus_script==6?8000u:classic_focus_script==5?10500u:5000u)&&!complete){std::fputs("Pikachu/Pichu special coverage incomplete\n",stderr);std::_Exit(6);}}
    if(purin_hat_script){
        unsigned costume=std::getenv("MELEE_TEST_PURIN_COSTUME")?std::atoi(std::getenv("MELEE_TEST_PURIN_COSTUME")):1;
        if(costume<1||costume>3)std::_Exit(6);
        for(unsigned i=0;i<costume;i++)if(frames==1170+12*i||frames==1176+12*i)melee_startup_publish_combat(0,0,0,frames==1170+12*i?0x400:0);
    }
    if(purin_hat_script&&frames==2800&&!melee_startup_purin_hat_progress()){std::fputs("Jigglypuff costume hat missing\n",stderr);std::_Exit(6);}
    if(purin_specials_script){int complete=melee_startup_purin_specials(frames);if(frames==7000&&!complete){std::fputs("Jigglypuff specials or multi-jumps incomplete\n",stderr);std::_Exit(6);}}
    if(yoshi_capture_script){int complete=melee_startup_yoshi_capture(frames);if(frames==7000&&!complete){std::fputs("Yoshi capture/release incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(yoshi_specials_script){int complete=melee_startup_yoshi_specials(frames);if(frames==7000&&!complete){std::fputs("Yoshi specials or shield incomplete\n",stderr);std::_Exit(6);}}
    if(yoshi_script&&frames==((yoshi_capture_script||yoshi_specials_script)?7000u:rematch_script?6000u:2800u)&&!melee_startup_yoshi_progress()){std::fputs("Yoshi did not reach active match\n",stderr);std::_Exit(6);}
    if(purin_script&&frames==800)melee_startup_unlock_purin();
    if(purin_script&&frames==(purin_specials_script?7000u:rematch_script?6000u:2800u)&&!melee_startup_purin_progress()){std::fputs("Jigglypuff did not reach active match\n",stderr);std::_Exit(6);}
    if(pichu_script&&frames==800)melee_startup_unlock_pichu();
    if(pichu_script&&frames==(pikachu_specials_script?5000u:rematch_script?6000u:2800u)&&!melee_startup_pichu_progress()){std::fputs("Pichu did not reach active match\n",stderr);std::_Exit(6);}
    if(pikachu_script&&frames==(pikachu_specials_script?5000u:rematch_script?6000u:2800u)&&!melee_startup_pikachu_progress()){std::fputs("Pikachu did not reach active match\n",stderr);std::_Exit(6);}
    if(roy_script&&frames==800)melee_startup_unlock_roy();
    if(roy_script&&frames==(roy_specials_script?5000u:rematch_script?6000u:2800u)&&!melee_startup_roy_progress()){std::fputs("Roy did not reach active match\n",stderr);std::_Exit(6);}
    if(roy_specials_script){int complete=melee_startup_roy_specials(frames);if(frames==(classic_focus_script==18?(std::getenv("MELEE_TEST_ADVENTURE_KIRBY")?23000u:19000u):classic_focus_script==17?18000u:classic_focus_script==16?16500u:classic_focus_script==15?14500u:classic_focus_script==14?12500u:classic_focus_script==13?8500u:classic_focus_script==12?7500u:classic_focus_script==10?9000u:classic_focus_script==8?(std::getenv("MELEE_TEST_ALLSTAR_CAMPAIGN")?36000u:24000u):classic_focus_script==7?11500u:classic_focus_script==6?8000u:classic_focus_script==5?10500u:5000u)&&!complete){std::fputs("Roy special coverage incomplete\n",stderr);std::_Exit(6);}}
    if(marth_specials_script){int complete=melee_startup_marth_specials(frames);if(frames==(classic_focus_script==18?(std::getenv("MELEE_TEST_ADVENTURE_KIRBY")?23000u:19000u):classic_focus_script==17?18000u:classic_focus_script==16?16500u:classic_focus_script==15?14500u:classic_focus_script==14?12500u:classic_focus_script==13?8500u:classic_focus_script==12?7500u:classic_focus_script==10?9000u:classic_focus_script==8?(std::getenv("MELEE_TEST_ALLSTAR_CAMPAIGN")?36000u:24000u):classic_focus_script==7?11500u:classic_focus_script==6?8000u:classic_focus_script==5?10500u:5000u)&&!complete){std::fputs("Marth special coverage incomplete\n",stderr);std::_Exit(6);}}
    if(marth_script&&frames==800)melee_startup_unlock_marth();
    if(marth_script&&frames==(marth_specials_script?5000u:rematch_script?6000u:2800u)&&!melee_startup_marth_progress()){std::fputs("Marth did not reach active match\n",stderr);std::_Exit(6);}
    if(luigi_script&&frames==800)melee_startup_unlock_luigi();
    if(iceclimbers_script&&frames==1700&&capture_path){
        const char* original=capture_path;std::string paired_capture=std::string(original)+".pair.png";
        capture_path=paired_capture.c_str();bool captured=capture_frame();capture_path=original;
        if(!captured)std::_Exit(5);
    }
    if(iceclimbers_script){melee_startup_select_iceclimbers(frames);if(frames==(iceclimbers_specials_script?7000u:rematch_script?6000u:4000u)&&!melee_startup_iceclimbers_progress()){std::fputs("Ice Climbers pair did not reach active match\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(kirby_script){melee_startup_select_kirby(frames);if(frames==(kirby_copy_script?7000u:kirby_specials_script?7000u:rematch_script?6000u:4000u)&&!melee_startup_kirby_progress()){std::fputs("Kirby did not reach active match\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(gamewatch_script){if(frames==800)melee_startup_unlock_gamewatch();melee_startup_select_gamewatch(frames);if(frames==(gamewatch_specials_script?7000u:rematch_script?6000u:4000u)&&!melee_startup_gamewatch_progress()){std::fputs("Game & Watch did not reach active match\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(mewtwo_script){if(frames==800)melee_startup_unlock_mewtwo();melee_startup_select_mewtwo(frames);if(frames==(mewtwo_specials_script?7000u:rematch_script?6000u:4000u)&&!melee_startup_mewtwo_progress()){std::fputs("Mewtwo did not reach active match\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(peach_script){melee_startup_select_peach(frames);if(frames==(peach_specials_script?7000u:rematch_script?6000u:4000u)&&!melee_startup_peach_progress()){std::fputs("Peach did not reach active match\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(samus_script){melee_startup_select_samus(frames);if(frames==(samus_specials_script?7000u:rematch_script?6000u:4000u)&&!melee_startup_samus_progress()){std::fputs("Samus did not reach active match\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(link_script){if(young_link_script&&frames==800)melee_startup_unlock_young_link();melee_startup_select_link(frames,young_link_script);if(frames==((kirby_specials_script||gamewatch_specials_script||iceclimbers_specials_script||mewtwo_specials_script||peach_specials_script||samus_specials_script||link_specials_script)?7000u:rematch_script?6000u:4000u)&&!melee_startup_link_progress(young_link_script)){std::fputs("Link did not reach active match\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(zelda_script){
        melee_startup_select_zelda(frames);
        int complete=zelda_specials_script?melee_startup_zelda_specials(frames):melee_startup_zelda_progress(frames,!rematch_script);
        if(frames==((kirby_specials_script||gamewatch_specials_script||iceclimbers_specials_script||mewtwo_specials_script||peach_specials_script||samus_specials_script||link_specials_script)?7000u:zelda_specials_script?8000u:rematch_script?6000u:5000u)&&!complete){std::fputs("Zelda/Sheik coverage incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}
    }
    if(yoshi_script)melee_startup_select_yoshi(frames);
    if(doctor_script){if(frames==800)melee_startup_unlock_doctor();melee_startup_select_doctor(frames);}
    if(ganon_script){if(frames==800)melee_startup_unlock_ganon();melee_startup_select_ganon(frames);}
    if(iceclimbers_specials_script){int complete=melee_startup_iceclimbers_specials(frames);if(frames==7000&&!complete){std::fputs("Ice Climbers specials incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(kirby_copy_script){melee_startup_kirby_copy_opponent(frames,kirby_copy_variant);int complete=melee_startup_kirby_copy(frames,kirby_copy_variant);if(frames==7000&&!complete){std::fputs("Kirby copy incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(kirby_specials_script){int complete=melee_startup_kirby_specials(frames);if(frames==7000&&!complete){std::fputs("Kirby specials incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(gamewatch_specials_script){int complete=melee_startup_gamewatch_specials(frames);if(frames==7000&&!complete){std::fputs("Game Watch specials incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(mewtwo_specials_script){int complete=melee_startup_mewtwo_specials(frames);if(frames==7000&&!complete){std::fputs("Mewtwo specials incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(peach_specials_script){int complete=melee_startup_peach_specials(frames);if(complete&&peach_float_script)complete=melee_startup_peach_float(frames);if(frames==7000&&!complete){std::fputs("Peach specials incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(samus_specials_script){int complete=melee_startup_samus_specials(frames);if(frames==7000&&!complete){std::fputs("Samus specials incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(link_specials_script){int complete=melee_startup_link_specials(frames,young_link_script,link_extras_script);if(frames==7000&&!complete){std::fputs("Link specials coverage incomplete\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}}
    if(select_script&&!kirby_script&&!gamewatch_script&&!iceclimbers_script&&!mewtwo_script&&!peach_script&&!samus_script&&!link_script&&!zelda_script&&!yoshi_script&&!doctor_script&&!ganon_script){
        if(frames==1020)melee_startup_publish_stick(0,80);
        if(frames==((purin_script||pichu_script||marth_script||roy_script)?1039u:(yoshi_script||pikachu_script||mario_script||captain_script||donkey_script||koopa_script||luigi_script)?1055u:1047u)||frames==(yoshi_script?1100u:purin_script?1088u:pikachu_script?1082u:pichu_script?1076u:roy_script?1111u:marth_script?1105u:luigi_script?1082u:koopa_script?1088u:donkey_script?1105u:captain_script?1111u:1076u))melee_startup_publish_stick(0,0);
        if(frames==1070)melee_startup_publish_stick(80,0);
        if(frames==((yoshi_script||purin_script||pikachu_script||pichu_script||roy_script||marth_script||captain_script||donkey_script||koopa_script||luigi_script)?1140u:1120u)||frames==((yoshi_script||purin_script||pikachu_script||pichu_script||roy_script||marth_script||captain_script||donkey_script||koopa_script||luigi_script)?1146u:1126u))melee_startup_publish_confirm(frames==((yoshi_script||purin_script||pikachu_script||pichu_script||roy_script||marth_script||captain_script||donkey_script||koopa_script||luigi_script)?1140u:1120u));
    }
    if(stage_script){
        if(frames==1320&&capture_path&&!capture_frame())std::_Exit(5);
        if(!kirby_copy_variant&&frames==1200)melee_startup_publish_opponent(0,80,0);
        if(!kirby_copy_variant&&(frames==1227||frames==1286))melee_startup_publish_opponent(0,0,0);
        if(!kirby_copy_variant&&frames==1280)melee_startup_publish_opponent(0,0,1);
        if(frames==1400||frames==1406)melee_startup_publish_start(frames==1400);
    }
    if(rematch_script){
        if(rcruise_script||kraid_script||flatzone_script||icemt_script||fourside_script||bigblue_script||shrine_script||venom_script||corneria_script||mutecity_script)melee_startup_results_rematch(frames);
        unsigned menu_frame=frames-((fountain_script||yorster_script||greens_script||shrine_script)?1600u:stadium_form_rematch_script?4400u:0u);
        int complete=melee_startup_rematch_progress(frames);
        if(frames==(icemt_script?10000u:shrine_script?11000u:venom_script?6000u:(fountain_script||yorster_script||greens_script)?8000u:stadium_form_rematch_script?10400u:6000u)&&!complete){std::fputs("Rematch did not reach a second active match\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}
        if(!rcruise_script&&!kraid_script&&!flatzone_script&&!icemt_script&&!fourside_script&&!bigblue_script&&!shrine_script&&!venom_script&&!corneria_script&&!mutecity_script&&(menu_frame==4000||menu_frame==4006)){
            melee_startup_publish_combat(0,0,0,menu_frame==4000?0x1000:0);
            melee_startup_publish_combat(1,0,0,menu_frame==4000?0x1000:0);
        }
        // Sudden Death can end after the generic combat inputs. Open the
        // score details first, then let both humans confirm the rematch.
        if((fountain_script||yorster_script||greens_script||stadium_form_rematch_script)&&(menu_frame==4200||menu_frame==4206)){
            melee_startup_publish_combat(0,0,0,menu_frame==4200?0x1000:0);
            melee_startup_publish_combat(1,0,0,menu_frame==4200?0x1000:0);
        }
        if(menu_frame==4300&&capture_path&&!capture_frame())std::_Exit(5);
        if(!rcruise_script&&!kraid_script&&!flatzone_script&&!icemt_script&&!fourside_script&&!bigblue_script&&!shrine_script&&!venom_script&&!corneria_script&&!mutecity_script&&(menu_frame==4400||menu_frame==4406))melee_startup_publish_start(menu_frame==4400);
    }
    if(finish_script){
        int complete=melee_startup_full_match_progress(frames);
        if(frames==12000&&!complete){std::fputs("Full match probe did not reach results with item coverage\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}
    }
    if(luigi_script&&frames>=1660){
        int complete=melee_startup_luigi_progress(frames,!rematch_script);
        if(frames==(rematch_script?6000u:4000u)&&!complete){std::fputs("Luigi probe did not observe required fighter, projectile and special moves\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}
    }
    if(koopa_script&&frames>=1660){
        int complete=melee_startup_koopa_progress(frames,!rematch_script);
        if(frames==(rematch_script?6000u:4000u)&&!complete){std::fputs("Bowser probe did not observe the required special moves\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}
    }
    if(donkey_script&&frames>=1660){
        int complete=melee_startup_donkey_progress(frames,!rematch_script);
        if(frames==(rematch_script?6000u:4000u)&&!complete){std::fputs("Donkey probe did not observe the required special moves\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}
    }
    if(captain_script&&frames>=1660){
        int complete=ganon_script?melee_startup_ganon_progress(frames,!rematch_script):melee_startup_captain_progress(frames,!rematch_script);
        if(frames==(rematch_script?6000u:4000u)&&!complete){std::fputs("Captain probe did not observe the required fighter/move state\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}
    }
    if(mario_script&&frames>=1660){
        int complete=doctor_script?melee_startup_doctor_progress(frames,!rematch_script):melee_startup_mario_progress(frames,!rematch_script);
        if(frames==(rematch_script?6000u:4000u)&&!complete){std::fputs("Mario probe did not observe its required fighter/projectile state\n",stderr);if(capture_path)capture_frame();std::_Exit(6);}
    }
    if(match_script){
        if(frames>=1660)melee_startup_report_fighters(frames);
        if(transition_script&&frames==((icemt_script&&rematch_script)||stadium_form_rematch_script?6200u:1850u)&&!melee_startup_shorten_match()){
            std::fputs("Transition probe did not reach an active timed match\n",stderr);
            if(capture_path)capture_frame();
            std::_Exit(6);
        }
        if(pokemon_script&&frames>=1850){
            int behavior=melee_startup_pokemon_progress(pokemon_script,frames);
            if(frames==((pokemon_script==17||pokemon_script==18)?5000u:pokemon_script==3?3600u:2800u)&&!behavior){std::fputs("Pokemon probe did not observe its expected behavior\n",stderr);std::_Exit(6);}
        }
        if(pokemon_script&&frames==1850&&!melee_startup_spawn_pokemon(pokemon_script)){if(capture_path)capture_frame();std::_Exit(6);}
        if(goldeen_script&&frames==1850&&!melee_startup_spawn_goldeen())std::_Exit(6);
        if(flower_script&&frames==1850&&!melee_startup_spawn_flower())std::_Exit(6);
        if(transition_script){
            unsigned transition_frame=frames-(stadium_form_rematch_script?4350u:0u);
            // Jump P2 off the stage before time expires so the results exercise
            // nonzero signed scores as well as the scene transition.
            if(transition_frame==1880)melee_startup_publish_combat(1,80,0,0x400);
            if(transition_frame==1886)melee_startup_publish_combat(1,80,0,0);
            if(transition_frame==2140)melee_startup_publish_combat(1,0,0,0);
            if(transition_frame==2800)melee_startup_publish_combat(1,-80,0,0);
            if(transition_frame==2860)melee_startup_publish_combat(1,0,0,0);
            if(transition_frame==2870)melee_startup_publish_combat(0,80,0,0);
            if(transition_frame==2890)melee_startup_publish_combat(0,0,0,0);
            if(transition_frame>=2900&&transition_frame<3500&&transition_frame%20==0)melee_startup_publish_combat(0,0,0,0x100);
            if(transition_frame>=2900&&transition_frame<3500&&transition_frame%20==6)melee_startup_publish_combat(0,0,0,0);
        }
        if(combat_script&&!kirby_copy_script&&!kirby_specials_script&&!gamewatch_specials_script&&!iceclimbers_specials_script&&!mewtwo_specials_script&&!peach_specials_script&&!samus_specials_script&&!zelda_specials_script&&!link_specials_script){
            if(frames==1690)melee_startup_publish_combat(1,-80,0,0);
            if(frames==1750)melee_startup_publish_combat(1,0,0,0);
            if(frames==1880)melee_startup_publish_combat(0,80,0,0);
            if(frames==1900)melee_startup_publish_combat(0,0,0,0);
            if(frames>=1920&&frames<2300&&frames%30==0)melee_startup_publish_combat(0,0,0,0x100);
            if(frames>=1920&&frames<2300&&frames%30==6)melee_startup_publish_combat(0,0,0,0);
            if(frames==2320)melee_startup_publish_combat(0,0,0,0x200);
            if(frames==2400)melee_startup_publish_combat(0,0,0,0x400);
            if(frames==2480)melee_startup_publish_combat(0,0,0,0x40);
            if(frames==2620)melee_startup_publish_combat(0,0,0,0x200);
            if(pause_script&&frames==2750)melee_startup_publish_combat(0,0,0,0x1000);
            if(pause_script&&frames==2756)melee_startup_publish_combat(0,0,0,0);
            if(frames==2720)melee_startup_publish_combat(0,0,-80,0x200);
            if(frames==2626||frames==2726)melee_startup_publish_combat(0,0,0,0);
            if(frames==2326||frames==2406||frames==2510)melee_startup_publish_combat(0,0,0,0);
        }
        if(luigi_script&&!rematch_script){
            if(frames==3000||frames==3550)melee_startup_publish_combat(0,-80,0,0x200);
            if(frames==3080||frames==3400)melee_startup_publish_combat(0,0,80,0x200);
            if(frames==3650||frames==3900)melee_startup_publish_combat(0,0,-80,0x200);
            if(frames==3556||frames==3006||frames==3086||frames==3406||frames==3656||frames==3906)melee_startup_publish_combat(0,0,0,0);
        }
        if(koopa_script&&!rematch_script){
            if(frames==3000||frames==3550)melee_startup_publish_combat(0,-80,0,0x200);
            if(frames==3080||frames==3400)melee_startup_publish_combat(0,0,80,0x200);
            if(frames==3650||frames==3900)melee_startup_publish_combat(0,0,-80,0x200);
            if(frames==3556||frames==3006||frames==3086||frames==3406||frames==3656||frames==3906)melee_startup_publish_combat(0,0,0,0);
        }
        if(donkey_script&&!rematch_script){
            if(frames==3000)melee_startup_publish_combat(0,-80,0,0x200);
            if(frames==3080||frames==3400)melee_startup_publish_combat(0,0,80,0x200);
            if(frames==3650||frames==3900)melee_startup_publish_combat(0,0,-80,0x200);
            if(frames==3006||frames==3086||frames==3406||frames==3656||frames==3906)melee_startup_publish_combat(0,0,0,0);
        }
        if(captain_script&&!rematch_script){
            if(frames==3000)melee_startup_publish_combat(0,-80,0,0x200);
            if(frames==3200||frames==3400)melee_startup_publish_combat(0,0,80,0x200);
            if(frames==3650)melee_startup_publish_combat(0,0,-80,0x200);
            if(frames==3006||frames==3206||frames==3406||frames==3656)melee_startup_publish_combat(0,0,0,0);
        }
        if(mario_script&&!rematch_script){
            if(frames==3000)melee_startup_publish_combat(0,80,0,0x200);
            if(frames==3080||frames==3400)melee_startup_publish_combat(0,0,80,0x200);
            if(frames==3550)melee_startup_publish_combat(0,0,-80,0x200);
            if(frames==3006||frames==3086||frames==3406||frames==3556)melee_startup_publish_combat(0,0,0,0);
        }
        if(frames==1700)melee_startup_publish_stick(80,0);
        if(frames==1730)melee_startup_publish_stick(0,0);
        if(frames==1800||frames==1806)melee_startup_publish_confirm(frames==1800);
        static unsigned phase=0, centered=0, confirmed_at=0;
        if(rematch_script&&frames==((fountain_script||yorster_script||greens_script||shrine_script)?5900u:stadium_form_rematch_script?8700u:4300u)){phase=0;centered=0;}
        unsigned stage_frames=mnStageSel_NativeFrames();
        if((rcruise_script||kraid_script||flatzone_script||icemt_script||fourside_script||bigblue_script||shrine_script||venom_script||corneria_script||mutecity_script)&&rematch_script&&!stage_frames){phase=0;centered=0;}
        if(stage_frames>=60&&phase==0){
            unsigned guidance=pura_script?mnStageSel_NativePuraGuidance():rcruise_script?mnStageSel_NativeRCruiseGuidance():kraid_script?mnStageSel_NativeKraidGuidance():flatzone_script?mnStageSel_NativeFlatzoneGuidance():icemt_script?mnStageSel_NativeIcemtGuidance():fourside_script?mnStageSel_NativeFoursideGuidance():bigblue_script?mnStageSel_NativeBigBlueGuidance():mutecity_script?mnStageSel_NativeMuteCityGuidance():corneria_script?mnStageSel_NativeCorneriaGuidance():venom_script?mnStageSel_NativeVenomGuidance():shrine_script?mnStageSel_NativeShrineGuidance():greens_script?mnStageSel_NativeGreensGuidance():yorster_script?mnStageSel_NativeYorsterGuidance():castle_script?mnStageSel_NativeCastleGuidance():brinstar_script?mnStageSel_NativeBrinstarGuidance():japes_script?mnStageSel_NativeJapesGuidance():kongo_script?mnStageSel_NativeKongoGuidance():greatbay_script?mnStageSel_NativeGreatbayGuidance():stadium_script?mnStageSel_NativeStadiumGuidance():story_script?mnStageSel_NativeStoryGuidance():fountain_script?mnStageSel_NativeFountainGuidance():dreamland_script?mnStageSel_NativeDreamlandGuidance():final_script?mnStageSel_NativeFinalGuidance():battle_script?mnStageSel_NativeBattleGuidance():mnStageSel_NativeOnettGuidance();
            if(guidance&16){
                int x=(guidance&1)?-45:(guidance&2)?45:0;
                int y=(guidance&4)?-45:(guidance&8)?45:0;
                melee_startup_publish_stick(x,y);
                centered=(x==0&&y==0)?centered+1:0;
                if(centered>=4){
                    melee_startup_publish_confirm(1);phase=1;confirmed_at=stage_frames;
                    std::fprintf(stderr,"Stage script confirming centered target at scene frame %u\n",stage_frames);
                }
            }
        }else if(phase==1){
            if(stage_frames==0||stage_frames>=confirmed_at+6){
                melee_startup_publish_confirm(0);phase=2;
            }else melee_startup_publish_confirm(1);
        }else if(stage_frames&&phase==2){
            melee_startup_publish_stick(0,0);
        }
    }
    if(pokemon_script==10&&frames>=1920&&frames<2400)melee_startup_target_wobbuffet(frames);
    if(frames==(classic_focus_script==18?(std::getenv("MELEE_TEST_ADVENTURE_KIRBY")?23000u:19000u):classic_focus_script==17?18000u:classic_focus_script==16?16500u:classic_focus_script==15?14500u:classic_focus_script==14?12500u:classic_focus_script==13?8500u:classic_focus_script==12?7500u:classic_focus_script==10?9000u:classic_focus_script==8?(std::getenv("MELEE_TEST_ALLSTAR_CAMPAIGN")?36000u:24000u):classic_focus_script==7?11500u:classic_focus_script==6?8000u:classic_focus_script==5?10500u:classic_focus_script?5000u:classic_continue_script?9000u:classic_gameover_script?4500u:classic_race_script?24000u:classic_team_script?21000u:classic_trophy_script?18000u:classic_fifth_script?15000u:hud_stamina_script?3200u:hud_restart_script?2800u:classic_after_bonus_script?12000u:classic_bonus_script?9000u:classic_round_script?6500u:classic_match_script?3800u:classic_entry_script?1800u:pura_platforms_script?7800u:rcruise_traverse_script?7800u:kraid_hazards_script?7800u:flatzone_hazards_script?7800u:icemt_bear_script?5000u:icemt_scroll_script?7800u:fourside_hazards_script?7800u:bigblue_track_script?7800u:mutecity_track_script?7800u:corneria_hazards_script?7800u:venom_hazards_script?7800u:greens_blocks_script?6500u:greens_hazards_script?7800u:yorster_blocks_script?6500u:castle_hazards_script?8200u:brinstar_acid_script?6500u:japes_hazards_script?6500u:greatbay_hazards_script?7800u:stadium_transform_script?8500u:kirby_copy_script?7000u:(kirby_specials_script||gamewatch_specials_script||iceclimbers_specials_script||mewtwo_specials_script||peach_specials_script||samus_specials_script||link_specials_script)?7000u:zelda_specials_script?8000u:(yoshi_capture_script||yoshi_specials_script||purin_specials_script)?7000u:event_preview_script?4500u:(pikachu_specials_script||marth_specials_script||roy_specials_script)?5000u:(story_hazards_script||fountain_platform_script||dreamland_wind_script||battle_background_script||final_background_script)?6500u:rematch_script?(icemt_script?10000u:shrine_script?11000u:venom_script?6000u:(fountain_script||yorster_script||greens_script)?8000u:stadium_form_rematch_script?10400u:6000u):transition_script?4000u:zelda_script?5000u:(kirby_script||gamewatch_script||iceclimbers_script||mewtwo_script||peach_script||samus_script||link_script||mario_script||captain_script||donkey_script||koopa_script||luigi_script)?4000u:finish_script?12000u:(pokemon_script==17||pokemon_script==18)?5000u:pokemon_script==3?3600u:combat_script?2800u:match_script?1900u:stage_script?1600u:select_script?1320u:versus_script?1200u:menu_script?900u:confirm_script?600u:120u)){if(capture_path&&!capture_frame())std::_Exit(5);if(audio_script)melee_startup_audio_finish();std::fputs("Startup probe reached its submission limit\n",stderr);std::_Exit(0);}
    if(!gfx::begin_frame())std::_Exit(4);
    gx::fifo::begin_frame();
}
void reset(MeleeResetRequest request,void*) {
    std::fprintf(stderr,"Original game requested reset kind=%d code=%u menu=%d\n",
        request.kind,request.code,request.force_menu);
    std::_Exit(90+request.kind);
}
}
int main(int argc,char** argv) {
    if(argc==2&&!std::strcmp(argv[1],"--empty-demo-motion"))return melee_startup_empty_demo_motion_test();
    if(argc==2&&!std::strcmp(argv[1],"--hand-laser-state"))return melee_startup_hand_laser_state_test();
    if(argc==2&&!std::strcmp(argv[1],"--allstar-roster"))return ifStock_NativeAllstarRosterTest();
    if(argc==2&&!std::strcmp(argv[1],"--stage-contact"))return grHeal_NativeContactTest()||grPushOn_NativeContactTest()||grIceMt_NativeContactFlagsTest();
    if(argc==2&&!std::strcmp(argv[1],"--icicle-contact"))return grIceMt_NativeContactFlagsTest();
    if(argc==2&&!std::strcmp(argv[1],"--ottosea-link"))return melee_startup_ottosea_link_test();
    if(argc==2&&!std::strcmp(argv[1],"--spawn-state"))return melee_startup_spawn_state_test();
    if(argc==2&&!std::strcmp(argv[1],"--player-mapping"))return melee_startup_player_mapping_test();
    if(argc==3&&!std::strcmp(argv[1],"--classic-layout"))return melee_startup_classic_layout_test(argv[2]);
    if(argc==2&&!std::strcmp(argv[1],"--pura-toon"))return melee_startup_pura_toon_test();
    if(argc==2&&!std::strcmp(argv[1],"--bigblue-car-lanes"))return grBigBlue_NativeCarLaneTest();
    if(argc==2&&!std::strcmp(argv[1],"--kirby-cache-reset"))return melee_startup_kirby_cache_reset_test();
    if(argc<3||argc>5||(argc==5&&std::strcmp(argv[4],"--intro-mask")&&std::strcmp(argv[4],"--classic-race-entry")&&std::strcmp(argv[4],"--adventure-brinstar-escape")&&std::strcmp(argv[4],"--adventure-maze-exit")&&std::strcmp(argv[4],"--adventure-maze-link")&&std::strcmp(argv[4],"--adventure-maze")&&std::strcmp(argv[4],"--adventure-jungle")&&std::strcmp(argv[4],"--adventure-course-exit")&&std::strcmp(argv[4],"--adventure-yoshis")&&std::strcmp(argv[4],"--adventure-blocks")&&std::strcmp(argv[4],"--adventure-traverse")&&std::strcmp(argv[4],"--adventure-entry")&&std::strcmp(argv[4],"--classic-campaign")&&std::strcmp(argv[4],"--classic-unlock-win")&&std::strcmp(argv[4],"--classic-ending-exit")&&std::strcmp(argv[4],"--classic-credits-complete")&&std::strcmp(argv[4],"--classic-boss-defeat")&&std::strcmp(argv[4],"--classic-boss-lasers")&&std::strcmp(argv[4],"--classic-boss-entry")&&std::strcmp(argv[4],"--classic-continue")&&std::strcmp(argv[4],"--classic-gameover")&&std::strcmp(argv[4],"--classic-race")&&std::strcmp(argv[4],"--classic-team")&&std::strcmp(argv[4],"--classic-trophy")&&std::strcmp(argv[4],"--classic-fifth")&&std::strcmp(argv[4],"--hud-stamina")&&std::strcmp(argv[4],"--hud-restart")&&std::strcmp(argv[4],"--classic-after-bonus")&&std::strcmp(argv[4],"--classic-bonus")&&std::strcmp(argv[4],"--classic-round")&&std::strcmp(argv[4],"--classic-match")&&std::strcmp(argv[4],"--classic-entry")&&std::strcmp(argv[4],"--confirm")&&std::strcmp(argv[4],"--event-preview")&&std::strcmp(argv[4],"--title")&&std::strcmp(argv[4],"--menu")&&std::strcmp(argv[4],"--versus")&&std::strcmp(argv[4],"--select")&&std::strcmp(argv[4],"--stage")&&std::strcmp(argv[4],"--match")&&std::strcmp(argv[4],"--mario-rematch")&&std::strcmp(argv[4],"--captain-rematch")&&std::strcmp(argv[4],"--donkey-rematch")&&std::strcmp(argv[4],"--koopa-rematch")&&std::strcmp(argv[4],"--luigi-rematch")&&std::strcmp(argv[4],"--battlefield-rematch")&&std::strcmp(argv[4],"--battlefield-background")&&std::strcmp(argv[4],"--final-destination-rematch")&&std::strcmp(argv[4],"--final-destination-background")&&std::strcmp(argv[4],"--final-destination-finish")&&std::strcmp(argv[4],"--final-destination")&&std::strcmp(argv[4],"--dreamland-rematch")&&std::strcmp(argv[4],"--dreamland-wind")&&std::strcmp(argv[4],"--fountain-rematch")&&std::strcmp(argv[4],"--fountain-platforms")&&std::strcmp(argv[4],"--yoshis-story-rematch")&&std::strcmp(argv[4],"--yoshis-story-hazards")&&std::strcmp(argv[4],"--stadium-fire")&&std::strcmp(argv[4],"--stadium-grass")&&std::strcmp(argv[4],"--stadium-water")&&std::strcmp(argv[4],"--stadium-form-rematch")&&std::strcmp(argv[4],"--pokemon-stadium-rematch")&&std::strcmp(argv[4],"--pokemon-stadium-transform")&&std::strcmp(argv[4],"--great-bay-hazards")&&std::strcmp(argv[4],"--great-bay-rematch")&&std::strcmp(argv[4],"--kongo-jungle-rematch")&&std::strcmp(argv[4],"--jungle-japes-hazards")&&std::strcmp(argv[4],"--jungle-japes-rematch")&&std::strcmp(argv[4],"--brinstar-rematch")&&std::strcmp(argv[4],"--brinstar-acid")&&std::strcmp(argv[4],"--peachs-castle-rematch")&&std::strcmp(argv[4],"--peachs-castle-hazards")&&std::strcmp(argv[4],"--yoshis-island-blocks")&&std::strcmp(argv[4],"--yoshis-island-rematch")&&std::strcmp(argv[4],"--green-greens-hazards")&&std::strcmp(argv[4],"--green-greens-rematch")&&std::strcmp(argv[4],"--green-greens-blocks")&&std::strcmp(argv[4],"--temple-rematch")&&std::strcmp(argv[4],"--venom-rematch")&&std::strcmp(argv[4],"--venom-hazards")&&std::strcmp(argv[4],"--corneria-rematch")&&std::strcmp(argv[4],"--corneria-hazards")&&std::strcmp(argv[4],"--mute-city-rematch")&&std::strcmp(argv[4],"--mute-city-track")&&std::strcmp(argv[4],"--big-blue-track")&&std::strcmp(argv[4],"--big-blue-rematch")&&std::strcmp(argv[4],"--fourside-hazards")&&std::strcmp(argv[4],"--fourside-rematch")&&std::strcmp(argv[4],"--icicle-mountain-scroll")&&std::strcmp(argv[4],"--icicle-mountain-rematch")&&std::strcmp(argv[4],"--flat-zone-rematch")&&std::strcmp(argv[4],"--brinstar-depths-hazards")&&std::strcmp(argv[4],"--brinstar-depths-rematch")&&std::strcmp(argv[4],"--rainbow-cruise-traverse")&&std::strcmp(argv[4],"--rainbow-cruise-rematch")&&std::strcmp(argv[4],"--poke-floats-platforms")&&std::strcmp(argv[4],"--poke-floats")&&std::strcmp(argv[4],"--rainbow-cruise")&&std::strcmp(argv[4],"--brinstar-depths")&&std::strcmp(argv[4],"--flat-zone-hazards")&&std::strcmp(argv[4],"--flat-zone")&&std::strcmp(argv[4],"--icicle-mountain-bear")&&std::strcmp(argv[4],"--icicle-mountain")&&std::strcmp(argv[4],"--fourside")&&std::strcmp(argv[4],"--big-blue")&&std::strcmp(argv[4],"--mute-city")&&std::strcmp(argv[4],"--corneria")&&std::strcmp(argv[4],"--venom")&&std::strcmp(argv[4],"--temple")&&std::strcmp(argv[4],"--green-greens")&&std::strcmp(argv[4],"--yoshis-island")&&std::strcmp(argv[4],"--peachs-castle")&&std::strcmp(argv[4],"--brinstar")&&std::strcmp(argv[4],"--jungle-japes")&&std::strcmp(argv[4],"--kongo-jungle")&&std::strcmp(argv[4],"--great-bay")&&std::strcmp(argv[4],"--pokemon-stadium")&&std::strcmp(argv[4],"--yoshis-story")&&std::strcmp(argv[4],"--fountain")&&std::strcmp(argv[4],"--dreamland")&&std::strcmp(argv[4],"--battlefield")&&std::strcmp(argv[4],"--roy-rematch")&&std::strcmp(argv[4],"--roy-specials")&&std::strcmp(argv[4],"--pikachu-specials")&&std::strcmp(argv[4],"--pichu-specials")&&std::strcmp(argv[4],"--pikachu-rematch")&&std::strcmp(argv[4],"--pichu-rematch")&&std::strcmp(argv[4],"--pikachu")&&std::strcmp(argv[4],"--jigglypuff-hat")&&std::strcmp(argv[4],"--jigglypuff-specials")&&std::strcmp(argv[4],"--jigglypuff-rematch")&&std::strcmp(argv[4],"--yoshi-rematch")&&std::strcmp(argv[4],"--yoshi-specials")&&std::strcmp(argv[4],"--yoshi-capture")&&std::strcmp(argv[4],"--yoshi")&&std::strcmp(argv[4],"--jigglypuff")&&std::strcmp(argv[4],"--pichu")&&std::strcmp(argv[4],"--roy")&&std::strcmp(argv[4],"--marth-rematch")&&std::strcmp(argv[4],"--marth-specials")&&std::strcmp(argv[4],"--marth")&&std::strcmp(argv[4],"--luigi")&&std::strcmp(argv[4],"--koopa")&&std::strcmp(argv[4],"--donkey")&&std::strcmp(argv[4],"--zelda-specials")&&std::strcmp(argv[4],"--zelda-rematch")&&std::strcmp(argv[4],"--link-rematch")&&std::strcmp(argv[4],"--young-link-rematch")&&std::strcmp(argv[4],"--link-extras")&&std::strcmp(argv[4],"--young-link-extras")&&std::strcmp(argv[4],"--samus-specials")&&std::strcmp(argv[4],"--samus-rematch")&&std::strcmp(argv[4],"--peach-rematch")&&std::strcmp(argv[4],"--peach-float")&&std::strcmp(argv[4],"--peach-specials")&&std::strcmp(argv[4],"--mewtwo-specials")&&std::strcmp(argv[4],"--mewtwo-rematch")&&std::strcmp(argv[4],"--ice-climbers-specials")&&std::strcmp(argv[4],"--ice-climbers-rematch")&&std::strcmp(argv[4],"--ice-climbers")&&std::strcmp(argv[4],"--game-watch-specials")&&std::strcmp(argv[4],"--game-watch-rematch")&&std::strcmp(argv[4],"--kirby-rematch")&&std::strcmp(argv[4],"--kirby-specials")&&std::strcmp(argv[4],"--kirby-copy-mario")&&std::strcmp(argv[4],"--kirby-copy-doctor")&&std::strcmp(argv[4],"--kirby-copy-luigi")&&std::strcmp(argv[4],"--kirby-copy-gamewatch")&&std::strcmp(argv[4],"--kirby-copy-yoshi")&&std::strcmp(argv[4],"--kirby-copy-purin")&&std::strcmp(argv[4],"--kirby-copy-link")&&std::strcmp(argv[4],"--kirby-copy-young-link")&&std::strcmp(argv[4],"--kirby-copy-mewtwo")&&std::strcmp(argv[4],"--kirby-copy-koopa")&&std::strcmp(argv[4],"--kirby-copy-pikachu")&&std::strcmp(argv[4],"--kirby-copy-pichu")&&std::strcmp(argv[4],"--kirby-copy-samus")&&std::strcmp(argv[4],"--kirby-copy-ice")&&std::strcmp(argv[4],"--kirby-copy-peach")&&std::strcmp(argv[4],"--kirby-copy-sheik")&&std::strcmp(argv[4],"--kirby-copy-zelda")&&std::strcmp(argv[4],"--kirby-copy-marth")&&std::strcmp(argv[4],"--kirby-copy-roy")&&std::strcmp(argv[4],"--kirby-copy-ganon")&&std::strcmp(argv[4],"--kirby-copy-captain")&&std::strcmp(argv[4],"--kirby-copy-donkey")&&std::strcmp(argv[4],"--kirby-copy-falco")&&std::strcmp(argv[4],"--kirby-copy-fox")&&std::strcmp(argv[4],"--kirby-copy")&&std::strcmp(argv[4],"--kirby")&&std::strcmp(argv[4],"--game-watch")&&std::strcmp(argv[4],"--mewtwo")&&std::strcmp(argv[4],"--peach")&&std::strcmp(argv[4],"--samus")&&std::strcmp(argv[4],"--link-specials")&&std::strcmp(argv[4],"--young-link-specials")&&std::strcmp(argv[4],"--young-link")&&std::strcmp(argv[4],"--link")&&std::strcmp(argv[4],"--zelda")&&std::strcmp(argv[4],"--ganondorf")&&std::strcmp(argv[4],"--ganondorf-rematch")&&std::strcmp(argv[4],"--captain")&&std::strcmp(argv[4],"--doctor-mario")&&std::strcmp(argv[4],"--doctor-mario-rematch")&&std::strcmp(argv[4],"--mario")&&std::strcmp(argv[4],"--combat")&&std::strcmp(argv[4],"--combat-audio")&&std::strcmp(argv[4],"--combat-pause")&&std::strcmp(argv[4],"--match-finish")&&std::strcmp(argv[4],"--flower")&&std::strcmp(argv[4],"--transition")&&std::strcmp(argv[4],"--rematch")&&std::strcmp(argv[4],"--goldeen")&&std::strcmp(argv[4],"--chikorita")&&std::strcmp(argv[4],"--snorlax")&&std::strcmp(argv[4],"--scizor")&&std::strcmp(argv[4],"--blastoise")&&std::strcmp(argv[4],"--weezing")&&std::strcmp(argv[4],"--charizard")&&std::strcmp(argv[4],"--moltres")&&std::strcmp(argv[4],"--zapdos")&&std::strcmp(argv[4],"--articuno")&&std::strcmp(argv[4],"--wobbuffet")&&std::strcmp(argv[4],"--bellossom")&&std::strcmp(argv[4],"--entei")&&std::strcmp(argv[4],"--raikou")&&std::strcmp(argv[4],"--suicune")&&std::strcmp(argv[4],"--electrode")&&std::strcmp(argv[4],"--unown")&&std::strcmp(argv[4],"--lugia")&&std::strcmp(argv[4],"--ho-oh")&&std::strcmp(argv[4],"--mew")&&std::strcmp(argv[4],"--cyndaquil")&&std::strcmp(argv[4],"--marill")&&std::strcmp(argv[4],"--venusaur")&&std::strcmp(argv[4],"--staryu")&&std::strcmp(argv[4],"--chansey")&&std::strcmp(argv[4],"--porygon2")&&std::strcmp(argv[4],"--clefairy")&&std::strcmp(argv[4],"--togepi")&&std::strcmp(argv[4],"--celebi"))){std::fputs("Usage: melee_game_startup IMAGE CACHE_DIRECTORY [OUTPUT_PNG [--classic-after-bonus|--classic-bonus|--classic-round|--classic-match|--classic-entry|--confirm|--event-preview|--title|--menu|--versus|--select|--stage|--match|--pikachu-specials|--pichu-specials|--pikachu-rematch|--pichu-rematch|--pikachu|--jigglypuff-hat|--jigglypuff-specials|--jigglypuff-rematch|--yoshi-rematch|--yoshi-specials|--yoshi-capture|--yoshi|--jigglypuff|--pichu|--roy|--roy-specials|--roy-rematch|--marth-specials|--marth|--marth-rematch|--final-destination-finish|--final-destination-background|--final-destination|--final-destination-rematch|--dreamland-rematch|--dreamland-wind|--fountain-rematch|--fountain-platforms|--yoshis-story-rematch|--yoshis-story-hazards|--stadium-fire|--stadium-grass|--stadium-water|--stadium-form-rematch|--pokemon-stadium-rematch|--pokemon-stadium-transform|--great-bay-hazards|--great-bay-rematch|--kongo-jungle-rematch|--jungle-japes-hazards|--jungle-japes-rematch|--brinstar-rematch|--brinstar-acid|--peachs-castle-rematch|--peachs-castle-hazards|--yoshis-island-blocks|--yoshis-island-rematch|--green-greens-hazards|--green-greens-rematch|--green-greens-blocks|--temple-rematch|--venom-rematch|--venom-hazards|--corneria-rematch|--corneria-hazards|--mute-city-rematch|--mute-city-track|--big-blue-track|--big-blue-rematch|--fourside-hazards|--fourside-rematch|--icicle-mountain-scroll|--icicle-mountain-rematch|--flat-zone-rematch|--brinstar-depths-hazards|--brinstar-depths-rematch|--rainbow-cruise-traverse|--rainbow-cruise-rematch|--poke-floats-platforms|--poke-floats|--rainbow-cruise|--brinstar-depths|--flat-zone-hazards|--flat-zone|--icicle-mountain-bear|--icicle-mountain|--fourside|--big-blue|--mute-city|--corneria|--venom|--temple|--green-greens|--yoshis-island|--peachs-castle|--brinstar|--jungle-japes|--kongo-jungle|--great-bay|--pokemon-stadium|--yoshis-story|--fountain|--dreamland|--battlefield|--battlefield-background|--battlefield-rematch|--luigi|--luigi-rematch|--koopa|--koopa-rematch|--donkey|--donkey-rematch|--zelda-specials|--zelda-rematch|--link-rematch|--young-link-rematch|--link-extras|--young-link-extras|--samus-specials|--samus-rematch|--peach-rematch|--peach-float|--peach-specials|--mewtwo-specials|--mewtwo-rematch|--ice-climbers-specials|--ice-climbers-rematch|--ice-climbers|--game-watch-specials|--game-watch-rematch|--kirby-rematch|--kirby-specials|--kirby-copy-mario|--kirby-copy-doctor|--kirby-copy-luigi|--kirby-copy-gamewatch|--kirby-copy-yoshi|--kirby-copy-purin|--kirby-copy-link|--kirby-copy-young-link|--kirby-copy-mewtwo|--kirby-copy-koopa|--kirby-copy-pikachu|--kirby-copy-pichu|--kirby-copy-samus|--kirby-copy-ice|--kirby-copy-peach|--kirby-copy-sheik|--kirby-copy-zelda|--kirby-copy-marth|--kirby-copy-roy|--kirby-copy-ganon|--kirby-copy-captain|--kirby-copy-donkey|--kirby-copy-falco|--kirby-copy-fox|--kirby-copy|--kirby|--game-watch|--mewtwo|--peach|--samus|--link-specials|--young-link-specials|--young-link|--link|--zelda|--ganondorf|--ganondorf-rematch|--captain|--captain-rematch|--doctor-mario|--doctor-mario-rematch|--mario|--mario-rematch|--combat|--combat-audio|--combat-pause|--match-finish|--flower|--transition|--rematch|--goldeen|--chikorita|--snorlax|--scizor|--blastoise|--weezing|--charizard|--moltres|--zapdos|--articuno|--wobbuffet|--bellossom|--entei|--raikou|--suicune|--electrode|--unown|--lugia|--ho-oh|--mew|--celebi|--cyndaquil|--marill|--venusaur|--staryu|--chansey|--porygon2|--clefairy|--togepi]]\n",stderr);return 2;}
    debug_stop=std::getenv("MELEE_STARTUP_DEBUG_STOP")!=nullptr;
    capture_path=argc>=4?argv[3]:nullptr;
    confirm_script=argc==5;
    intro_mask_script=argc==5&&!std::strcmp(argv[4],"--intro-mask");
    classic_race_script=argc==5&&!std::strcmp(argv[4],"--classic-race");
    classic_team_script=classic_race_script||(argc==5&&!std::strcmp(argv[4],"--classic-team"));
    classic_trophy_script=classic_team_script||(argc==5&&!std::strcmp(argv[4],"--classic-trophy"));
    classic_fifth_script=classic_trophy_script||(argc==5&&!std::strcmp(argv[4],"--classic-fifth"));
    classic_after_bonus_script=classic_fifth_script||(argc==5&&!std::strcmp(argv[4],"--classic-after-bonus"));
    classic_bonus_script=classic_after_bonus_script||(argc==5&&!std::strcmp(argv[4],"--classic-bonus"));
    classic_round_script=classic_bonus_script||(argc==5&&!std::strcmp(argv[4],"--classic-round"));
    classic_focus_script=argc==5&&!std::strcmp(argv[4],"--adventure-brinstar-escape")?18:argc==5&&!std::strcmp(argv[4],"--adventure-maze-exit")?17:argc==5&&!std::strcmp(argv[4],"--adventure-maze-link")?16:argc==5&&!std::strcmp(argv[4],"--adventure-maze")?15:argc==5&&!std::strcmp(argv[4],"--adventure-jungle")?14:argc==5&&!std::strcmp(argv[4],"--adventure-course-exit")?13:argc==5&&!std::strcmp(argv[4],"--adventure-yoshis")?12:argc==5&&!std::strcmp(argv[4],"--adventure-blocks")?11:argc==5&&!std::strcmp(argv[4],"--adventure-traverse")?10:argc==5&&!std::strcmp(argv[4],"--adventure-entry")?9:argc==5&&!std::strcmp(argv[4],"--classic-campaign")?8:argc==5&&!std::strcmp(argv[4],"--classic-unlock-win")?7:argc==5&&!std::strcmp(argv[4],"--classic-ending-exit")?6:argc==5&&!std::strcmp(argv[4],"--classic-credits-complete")?5:argc==5&&!std::strcmp(argv[4],"--classic-boss-defeat")?4:argc==5&&!std::strcmp(argv[4],"--classic-boss-lasers")?3:argc==5&&!std::strcmp(argv[4],"--classic-race-entry")?1:argc==5&&!std::strcmp(argv[4],"--classic-boss-entry")?2:0;
    classic_continue_script=argc==5&&!std::strcmp(argv[4],"--classic-continue");
    classic_gameover_script=classic_continue_script||(argc==5&&!std::strcmp(argv[4],"--classic-gameover"));
    classic_match_script=classic_focus_script||classic_gameover_script||classic_round_script||(argc==5&&!std::strcmp(argv[4],"--classic-match"));
    classic_entry_script=classic_match_script||(argc==5&&!std::strcmp(argv[4],"--classic-entry"));
    audio_script=argc==5&&!std::strcmp(argv[4],"--combat-audio");
    pause_script=argc==5&&!std::strcmp(argv[4],"--combat-pause");
    finish_script=argc==5&&(!std::strcmp(argv[4],"--match-finish")||!std::strcmp(argv[4],"--final-destination-finish"));
    pokemon_script=argc==5?(!std::strcmp(argv[4],"--chikorita")?1:!std::strcmp(argv[4],"--snorlax")?2:!std::strcmp(argv[4],"--scizor")?3:!std::strcmp(argv[4],"--blastoise")?4:!std::strcmp(argv[4],"--weezing")?5:!std::strcmp(argv[4],"--charizard")?6:!std::strcmp(argv[4],"--moltres")?7:!std::strcmp(argv[4],"--zapdos")?8:!std::strcmp(argv[4],"--articuno")?9:!std::strcmp(argv[4],"--wobbuffet")?10:!std::strcmp(argv[4],"--bellossom")?11:!std::strcmp(argv[4],"--entei")?12:!std::strcmp(argv[4],"--raikou")?13:!std::strcmp(argv[4],"--suicune")?14:!std::strcmp(argv[4],"--electrode")?15:!std::strcmp(argv[4],"--unown")?16:!std::strcmp(argv[4],"--lugia")?17:!std::strcmp(argv[4],"--ho-oh")?18:!std::strcmp(argv[4],"--mew")?19:!std::strcmp(argv[4],"--celebi")?20:!std::strcmp(argv[4],"--cyndaquil")?21:!std::strcmp(argv[4],"--marill")?22:!std::strcmp(argv[4],"--venusaur")?23:!std::strcmp(argv[4],"--staryu")?24:!std::strcmp(argv[4],"--chansey")?25:!std::strcmp(argv[4],"--porygon2")?26:!std::strcmp(argv[4],"--clefairy")?27:!std::strcmp(argv[4],"--togepi")?28:0):0;
    goldeen_script=argc==5&&!std::strcmp(argv[4],"--goldeen");
    flower_script=argc==5&&!std::strcmp(argv[4],"--flower");
    stadium_form_rematch_script=argc==5&&!std::strcmp(argv[4],"--stadium-form-rematch");
    rematch_script=stadium_form_rematch_script||argc==5&&(!std::strcmp(argv[4],"--rainbow-cruise-rematch")||!std::strcmp(argv[4],"--brinstar-depths-rematch")||!std::strcmp(argv[4],"--flat-zone-rematch")||!std::strcmp(argv[4],"--icicle-mountain-rematch")||!std::strcmp(argv[4],"--fourside-rematch")||!std::strcmp(argv[4],"--big-blue-rematch")||!std::strcmp(argv[4],"--mute-city-rematch")||!std::strcmp(argv[4],"--corneria-rematch")||!std::strcmp(argv[4],"--venom-rematch")||!std::strcmp(argv[4],"--temple-rematch")||!std::strcmp(argv[4],"--green-greens-rematch")||!std::strcmp(argv[4],"--yoshis-island-rematch")||!std::strcmp(argv[4],"--peachs-castle-rematch")||!std::strcmp(argv[4],"--brinstar-rematch")||!std::strcmp(argv[4],"--jungle-japes-rematch")||!std::strcmp(argv[4],"--kongo-jungle-rematch")||!std::strcmp(argv[4],"--great-bay-rematch")||!std::strcmp(argv[4],"--pokemon-stadium-rematch")||!std::strcmp(argv[4],"--yoshis-story-rematch")||!std::strcmp(argv[4],"--fountain-rematch")||!std::strcmp(argv[4],"--dreamland-rematch")||!std::strcmp(argv[4],"--kirby-rematch")||!std::strcmp(argv[4],"--game-watch-rematch")||!std::strcmp(argv[4],"--ice-climbers-rematch")||!std::strcmp(argv[4],"--mewtwo-rematch")||!std::strcmp(argv[4],"--peach-rematch")||!std::strcmp(argv[4],"--samus-rematch")||!std::strcmp(argv[4],"--link-rematch")||!std::strcmp(argv[4],"--young-link-rematch")||!std::strcmp(argv[4],"--zelda-rematch")||!std::strcmp(argv[4],"--ganondorf-rematch")||!std::strcmp(argv[4],"--doctor-mario-rematch")||!std::strcmp(argv[4],"--yoshi-rematch")||!std::strcmp(argv[4],"--jigglypuff-rematch")||!std::strcmp(argv[4],"--pikachu-rematch")||!std::strcmp(argv[4],"--pichu-rematch")||!std::strcmp(argv[4],"--roy-rematch")||!std::strcmp(argv[4],"--marth-rematch")||!std::strcmp(argv[4],"--final-destination-rematch")||!std::strcmp(argv[4],"--battlefield-rematch")||!std::strcmp(argv[4],"--luigi-rematch")||!std::strcmp(argv[4],"--koopa-rematch")||!std::strcmp(argv[4],"--rematch")||!std::strcmp(argv[4],"--donkey-rematch")||!std::strcmp(argv[4],"--captain-rematch")||!std::strcmp(argv[4],"--mario-rematch"));
    transition_script=rematch_script||(argc==5&&!std::strcmp(argv[4],"--transition"));
    doctor_script=argc==5&&(!std::strcmp(argv[4],"--doctor-mario")||!std::strcmp(argv[4],"--doctor-mario-rematch"));
    mario_script=doctor_script||(argc==5&&(!std::strcmp(argv[4],"--mario")||!std::strcmp(argv[4],"--mario-rematch")));
    link_extras_script=argc==5&&(!std::strcmp(argv[4],"--link-extras")||!std::strcmp(argv[4],"--young-link-extras"));
    link_specials_script=link_extras_script||argc==5&&(!std::strcmp(argv[4],"--link-specials")||!std::strcmp(argv[4],"--young-link-specials"));
    iceclimbers_specials_script=argc==5&&!std::strcmp(argv[4],"--ice-climbers-specials");
    iceclimbers_script=iceclimbers_specials_script||argc==5&&(!std::strcmp(argv[4],"--ice-climbers")||!std::strcmp(argv[4],"--ice-climbers-rematch"));
    kirby_specials_script=argc==5&&!std::strcmp(argv[4],"--kirby-specials");
    kirby_copy_variant=argc==5?(!std::strcmp(argv[4],"--kirby-copy-mario")?1:!std::strcmp(argv[4],"--kirby-copy-doctor")?2:!std::strcmp(argv[4],"--kirby-copy-luigi")?3:!std::strcmp(argv[4],"--kirby-copy-fox")?4:!std::strcmp(argv[4],"--kirby-copy-falco")?5:!std::strcmp(argv[4],"--kirby-copy-donkey")?6:!std::strcmp(argv[4],"--kirby-copy-captain")?7:!std::strcmp(argv[4],"--kirby-copy-ganon")?8:!std::strcmp(argv[4],"--kirby-copy-marth")?9:!std::strcmp(argv[4],"--kirby-copy-roy")?10:!std::strcmp(argv[4],"--kirby-copy-zelda")?11:!std::strcmp(argv[4],"--kirby-copy-sheik")?12:!std::strcmp(argv[4],"--kirby-copy-peach")?13:!std::strcmp(argv[4],"--kirby-copy-ice")?14:!std::strcmp(argv[4],"--kirby-copy-samus")?15:!std::strcmp(argv[4],"--kirby-copy-pikachu")?16:!std::strcmp(argv[4],"--kirby-copy-pichu")?17:!std::strcmp(argv[4],"--kirby-copy-koopa")?18:!std::strcmp(argv[4],"--kirby-copy-mewtwo")?19:!std::strcmp(argv[4],"--kirby-copy-link")?20:!std::strcmp(argv[4],"--kirby-copy-young-link")?21:!std::strcmp(argv[4],"--kirby-copy-purin")?22:!std::strcmp(argv[4],"--kirby-copy-yoshi")?23:!std::strcmp(argv[4],"--kirby-copy-gamewatch")?24:0):0;
    kirby_copy_script=kirby_copy_variant||argc==5&&!std::strcmp(argv[4],"--kirby-copy");
    kirby_script=kirby_copy_script||kirby_specials_script||argc==5&&(!std::strcmp(argv[4],"--kirby")||!std::strcmp(argv[4],"--kirby-rematch"));
    gamewatch_specials_script=argc==5&&!std::strcmp(argv[4],"--game-watch-specials");
    gamewatch_script=gamewatch_specials_script||argc==5&&(!std::strcmp(argv[4],"--game-watch")||!std::strcmp(argv[4],"--game-watch-rematch"));
    mewtwo_specials_script=argc==5&&!std::strcmp(argv[4],"--mewtwo-specials");
    mewtwo_script=mewtwo_specials_script||argc==5&&(!std::strcmp(argv[4],"--mewtwo")||!std::strcmp(argv[4],"--mewtwo-rematch"));
    peach_float_script=argc==5&&!std::strcmp(argv[4],"--peach-float");
    peach_specials_script=peach_float_script||argc==5&&!std::strcmp(argv[4],"--peach-specials");
    peach_script=peach_specials_script||argc==5&&(!std::strcmp(argv[4],"--peach")||!std::strcmp(argv[4],"--peach-rematch"));
    samus_specials_script=argc==5&&!std::strcmp(argv[4],"--samus-specials");
    samus_script=samus_specials_script||argc==5&&(!std::strcmp(argv[4],"--samus")||!std::strcmp(argv[4],"--samus-rematch"));
    young_link_script=argc==5&&(!std::strcmp(argv[4],"--young-link-extras")||!std::strcmp(argv[4],"--young-link-rematch")||!std::strcmp(argv[4],"--young-link")||!std::strcmp(argv[4],"--young-link-specials"));
    link_script=link_specials_script||young_link_script||argc==5&&(!std::strcmp(argv[4],"--link")||!std::strcmp(argv[4],"--link-rematch"));
    zelda_specials_script=argc==5&&!std::strcmp(argv[4],"--zelda-specials");
    zelda_script=zelda_specials_script||argc==5&&(!std::strcmp(argv[4],"--zelda")||!std::strcmp(argv[4],"--zelda-rematch"));
    ganon_script=argc==5&&(!std::strcmp(argv[4],"--ganondorf")||!std::strcmp(argv[4],"--ganondorf-rematch"));
    captain_script=ganon_script||(argc==5&&(!std::strcmp(argv[4],"--captain")||!std::strcmp(argv[4],"--captain-rematch")));
    donkey_script=argc==5&&(!std::strcmp(argv[4],"--donkey")||!std::strcmp(argv[4],"--donkey-rematch"));
    koopa_script=argc==5&&(!std::strcmp(argv[4],"--koopa")||!std::strcmp(argv[4],"--koopa-rematch"));
    luigi_script=argc==5&&(!std::strcmp(argv[4],"--luigi")||!std::strcmp(argv[4],"--luigi-rematch"));
    battle_background_script=argc==5&&!std::strcmp(argv[4],"--battlefield-background");
    dreamland_wind_script=argc==5&&!std::strcmp(argv[4],"--dreamland-wind");
    fountain_platform_script=argc==5&&!std::strcmp(argv[4],"--fountain-platforms");
    story_hazards_script=argc==5&&!std::strcmp(argv[4],"--yoshis-story-hazards");
    stadium_requested_form=argc==5?(!std::strcmp(argv[4],"--stadium-fire")?3:!std::strcmp(argv[4],"--stadium-grass")?4:!std::strcmp(argv[4],"--stadium-water")?9:0):0;
    stadium_transform_script=stadium_requested_form||argc==5&&!std::strcmp(argv[4],"--pokemon-stadium-transform");
    greatbay_hazards_script=argc==5&&!std::strcmp(argv[4],"--great-bay-hazards");
    japes_hazards_script=argc==5&&!std::strcmp(argv[4],"--jungle-japes-hazards");
    brinstar_acid_script=argc==5&&!std::strcmp(argv[4],"--brinstar-acid");
    castle_hazards_script=argc==5&&!std::strcmp(argv[4],"--peachs-castle-hazards");
    pura_platforms_script=argc==5&&!std::strcmp(argv[4],"--poke-floats-platforms");
    pura_script=pura_platforms_script||argc==5&&!std::strcmp(argv[4],"--poke-floats");
    rcruise_traverse_script=argc==5&&!std::strcmp(argv[4],"--rainbow-cruise-traverse");
    rcruise_script=rcruise_traverse_script||argc==5&&(!std::strcmp(argv[4],"--rainbow-cruise")||!std::strcmp(argv[4],"--rainbow-cruise-rematch"));
    kraid_hazards_script=argc==5&&!std::strcmp(argv[4],"--brinstar-depths-hazards");
    kraid_script=kraid_hazards_script||argc==5&&(!std::strcmp(argv[4],"--brinstar-depths")||!std::strcmp(argv[4],"--brinstar-depths-rematch"));
    flatzone_hazards_script=argc==5&&!std::strcmp(argv[4],"--flat-zone-hazards");
    flatzone_script=flatzone_hazards_script||argc==5&&(!std::strcmp(argv[4],"--flat-zone")||!std::strcmp(argv[4],"--flat-zone-rematch"));
    icemt_bear_script=argc==5&&!std::strcmp(argv[4],"--icicle-mountain-bear");
    icemt_scroll_script=argc==5&&!std::strcmp(argv[4],"--icicle-mountain-scroll");
    icemt_script=icemt_bear_script||icemt_scroll_script||argc==5&&(!std::strcmp(argv[4],"--icicle-mountain")||!std::strcmp(argv[4],"--icicle-mountain-rematch"));
    fourside_hazards_script=argc==5&&!std::strcmp(argv[4],"--fourside-hazards");
    fourside_script=fourside_hazards_script||argc==5&&(!std::strcmp(argv[4],"--fourside")||!std::strcmp(argv[4],"--fourside-rematch"));
    bigblue_track_script=argc==5&&!std::strcmp(argv[4],"--big-blue-track");
    bigblue_script=bigblue_track_script||argc==5&&(!std::strcmp(argv[4],"--big-blue")||!std::strcmp(argv[4],"--big-blue-rematch"));
    mutecity_track_script=argc==5&&!std::strcmp(argv[4],"--mute-city-track");
    mutecity_script=mutecity_track_script||argc==5&&(!std::strcmp(argv[4],"--mute-city")||!std::strcmp(argv[4],"--mute-city-rematch"));
    corneria_hazards_script=argc==5&&!std::strcmp(argv[4],"--corneria-hazards");
    corneria_script=corneria_hazards_script||argc==5&&(!std::strcmp(argv[4],"--corneria")||!std::strcmp(argv[4],"--corneria-rematch"));
    venom_hazards_script=argc==5&&!std::strcmp(argv[4],"--venom-hazards");
    venom_script=venom_hazards_script||argc==5&&(!std::strcmp(argv[4],"--venom")||!std::strcmp(argv[4],"--venom-rematch"));
    shrine_script=argc==5&&(!std::strcmp(argv[4],"--temple")||!std::strcmp(argv[4],"--temple-rematch"));
    greens_blocks_script=argc==5&&!std::strcmp(argv[4],"--green-greens-blocks");
    greens_hazards_script=argc==5&&!std::strcmp(argv[4],"--green-greens-hazards");
    greens_script=greens_blocks_script||greens_hazards_script||argc==5&&(!std::strcmp(argv[4],"--green-greens")||!std::strcmp(argv[4],"--green-greens-rematch"));
    yorster_blocks_script=argc==5&&!std::strcmp(argv[4],"--yoshis-island-blocks");
    yorster_script=yorster_blocks_script||argc==5&&(!std::strcmp(argv[4],"--yoshis-island")||!std::strcmp(argv[4],"--yoshis-island-rematch"));
    castle_script=castle_hazards_script||argc==5&&(!std::strcmp(argv[4],"--peachs-castle")||!std::strcmp(argv[4],"--peachs-castle-rematch"));
    brinstar_script=brinstar_acid_script||argc==5&&(!std::strcmp(argv[4],"--brinstar")||!std::strcmp(argv[4],"--brinstar-rematch"));
    japes_script=japes_hazards_script||argc==5&&(!std::strcmp(argv[4],"--jungle-japes")||!std::strcmp(argv[4],"--jungle-japes-rematch"));
    kongo_script=argc==5&&(!std::strcmp(argv[4],"--kongo-jungle")||!std::strcmp(argv[4],"--kongo-jungle-rematch"));
    greatbay_script=greatbay_hazards_script||argc==5&&(!std::strcmp(argv[4],"--great-bay")||!std::strcmp(argv[4],"--great-bay-rematch"));
    stadium_script=stadium_form_rematch_script||stadium_transform_script||argc==5&&(!std::strcmp(argv[4],"--pokemon-stadium")||!std::strcmp(argv[4],"--pokemon-stadium-rematch"));
    story_script=story_hazards_script||argc==5&&(!std::strcmp(argv[4],"--yoshis-story")||!std::strcmp(argv[4],"--yoshis-story-rematch"));
    fountain_script=fountain_platform_script||argc==5&&(!std::strcmp(argv[4],"--fountain")||!std::strcmp(argv[4],"--fountain-rematch"));
    dreamland_script=dreamland_wind_script||argc==5&&(!std::strcmp(argv[4],"--dreamland")||!std::strcmp(argv[4],"--dreamland-rematch"));
    battle_script=argc==5&&(!std::strcmp(argv[4],"--battlefield-background")||!std::strcmp(argv[4],"--battlefield")||!std::strcmp(argv[4],"--battlefield-rematch"));
    final_background_script=argc==5&&!std::strcmp(argv[4],"--final-destination-background");
    final_script=kirby_copy_script||argc==5&&(!std::strcmp(argv[4],"--final-destination-finish")||final_background_script||!std::strcmp(argv[4],"--final-destination")||!std::strcmp(argv[4],"--final-destination-rematch"));
    marth_specials_script=argc==5&&!std::strcmp(argv[4],"--marth-specials");
    marth_script=argc==5&&(marth_specials_script||!std::strcmp(argv[4],"--marth")||!std::strcmp(argv[4],"--marth-rematch"));
    roy_specials_script=argc==5&&!std::strcmp(argv[4],"--roy-specials");
    roy_script=argc==5&&(roy_specials_script||!std::strcmp(argv[4],"--roy")||!std::strcmp(argv[4],"--roy-rematch"));
    pikachu_specials_script=argc==5&&(!std::strcmp(argv[4],"--pikachu-specials")||!std::strcmp(argv[4],"--pichu-specials"));
    pikachu_script=argc==5&&(!std::strcmp(argv[4],"--pikachu")||!std::strcmp(argv[4],"--pikachu-specials")||!std::strcmp(argv[4],"--pikachu-rematch"));
    pichu_script=argc==5&&(!std::strcmp(argv[4],"--pichu")||!std::strcmp(argv[4],"--pichu-specials")||!std::strcmp(argv[4],"--pichu-rematch"));
    purin_hat_script=argc==5&&!std::strcmp(argv[4],"--jigglypuff-hat");
    purin_specials_script=argc==5&&!std::strcmp(argv[4],"--jigglypuff-specials");
    purin_script=purin_specials_script||purin_hat_script||(argc==5&&(!std::strcmp(argv[4],"--jigglypuff")||!std::strcmp(argv[4],"--jigglypuff-rematch")));
    yoshi_specials_script=argc==5&&!std::strcmp(argv[4],"--yoshi-specials");
    yoshi_capture_script=argc==5&&!std::strcmp(argv[4],"--yoshi-capture");
    yoshi_script=yoshi_capture_script||yoshi_specials_script||(argc==5&&(!std::strcmp(argv[4],"--yoshi")||!std::strcmp(argv[4],"--yoshi-rematch")));
    combat_script=pura_script||rcruise_script||kraid_script||flatzone_script||icemt_script||fourside_script||bigblue_script||mutecity_script||corneria_script||venom_script||shrine_script||greens_script||yorster_script||castle_script||brinstar_script||japes_script||kongo_script||greatbay_script||stadium_script||story_script||fountain_script||dreamland_script||kirby_script||gamewatch_script||iceclimbers_script||mewtwo_script||peach_script||samus_script||link_script||zelda_script||yoshi_script||purin_script||pikachu_script||pichu_script||roy_script||marth_script||final_script||battle_script||luigi_script||koopa_script||donkey_script||captain_script||mario_script||pokemon_script||goldeen_script||transition_script||flower_script||finish_script||pause_script||audio_script||(argc==5&&!std::strcmp(argv[4],"--combat"));
    hud_stamina_script=argc==5&&!std::strcmp(argv[4],"--hud-stamina");
    hud_restart_script=hud_stamina_script||(argc==5&&!std::strcmp(argv[4],"--hud-restart"));
    match_script=hud_restart_script||combat_script||(argc==5&&!std::strcmp(argv[4],"--match"));
    stage_script=match_script||(argc==5&&!std::strcmp(argv[4],"--stage"));
    select_script=stage_script||(argc==5&&!std::strcmp(argv[4],"--select"));
    versus_script=select_script||(argc==5&&!std::strcmp(argv[4],"--versus"));
    event_preview_script=argc==5&&!std::strcmp(argv[4],"--event-preview");
    menu_script=classic_entry_script||event_preview_script||versus_script||(argc==5&&!std::strcmp(argv[4],"--menu"));
    title_script=menu_script||(argc==5&&!std::strcmp(argv[4],"--title"));
    if(confirm_script)melee_startup_publish_confirm(0);
    setvbuf(stdout,nullptr,_IONBF,0);setvbuf(stderr,nullptr,_IONBF,0);
    std::thread([]{std::this_thread::sleep_for(std::chrono::seconds(5));
        lbDvd_NativeReportPreloads();}).detach();
    std::thread([]{std::this_thread::sleep_for(std::chrono::seconds(classic_focus_script==18?(std::getenv("MELEE_TEST_ADVENTURE_KIRBY")?1200:900):classic_focus_script==17?840:classic_focus_script==16?780:classic_focus_script==15?660:classic_focus_script==14?600:classic_focus_script==13?480:classic_focus_script==12?420:classic_focus_script==10?480:classic_focus_script==8?(std::getenv("MELEE_TEST_ALLSTAR_CAMPAIGN")?2400:1200):classic_focus_script==7?540:classic_focus_script==6?420:classic_focus_script==5?540:classic_focus_script?240:classic_continue_script?420:classic_gameover_script?300:classic_race_script?1020:classic_team_script?900:classic_trophy_script?780:classic_fifth_script?660:hud_restart_script?180:classic_after_bonus_script?540:classic_bonus_script?420:classic_round_script?300:classic_entry_script?180:pura_platforms_script?360:rcruise_traverse_script?360:kraid_hazards_script?360:flatzone_hazards_script?360:icemt_script&&rematch_script?440:icemt_bear_script?240:icemt_scroll_script?360:fourside_hazards_script?360:bigblue_track_script?360:mutecity_track_script?360:corneria_hazards_script?360:venom_hazards_script?360:greens_blocks_script?300:greens_hazards_script?360:yorster_blocks_script?300:castle_hazards_script?360:brinstar_acid_script?300:japes_hazards_script?300:greatbay_hazards_script?360:stadium_form_rematch_script?440:stadium_transform_script?360:(kirby_copy_script||kirby_specials_script||gamewatch_specials_script||iceclimbers_specials_script||mewtwo_specials_script||peach_specials_script||samus_specials_script||link_specials_script||zelda_specials_script||yoshi_capture_script||yoshi_specials_script||purin_specials_script)?300:event_preview_script?180:(pikachu_specials_script||marth_specials_script||roy_specials_script)?240:(story_hazards_script||fountain_platform_script||dreamland_wind_script||battle_background_script||final_background_script)?300:finish_script?480:(pokemon_script==17||pokemon_script==18)?240:rematch_script?((shrine_script||venom_script)?440:(fountain_script||yorster_script||greens_script)?340:260):transition_script||mario_script||captain_script||donkey_script||koopa_script||luigi_script?180:debug_stop||match_script?120:stage_script?45:30));
        std::fputs("Startup probe timed out\n",stderr);std::_Exit(124);}).detach();
    if(!melee_dvd_mount(argv[1]))return 3;
    melee_reset_configure(0,reset,nullptr);
    using namespace aurora;
    const auto cache=std::filesystem::absolute(argv[2]).string();
    std::filesystem::create_directories(cache);
    g_config={};g_config.appName="Melee original startup probe";
    g_config.cachePath=cache.c_str();g_config.userPath=cache.c_str();g_config.resourcesPath=cache.c_str();
    g_config.msaa=1;g_config.maxTextureAnisotropy=4;
    if(!webgpu::initialize(BACKEND_METAL,false,640,480))return 4;
    gfx::initialize();gx::fifo::init();
    if(!gfx::begin_frame())return 4;
    gx::fifo::begin_frame();gx::fifo::set_draw_done_submitter(submit_frame);
    std::fputs("Entering original Melee startup\n",stderr);
    int result=melee_game_main();
    std::fprintf(stderr,"Original game entry returned %d\n",result);
    std::_Exit(result);
}
