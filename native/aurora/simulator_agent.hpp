#pragma once
// Simulator-only controller bridge. No networking or model code in the app.
#include <filesystem>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <thread>
extern "C" {
void melee_sim_agent_input(unsigned,float,float,float,float);
void melee_sim_agent_observe(char*,size_t);
}
class SimulatorAgent {
    std::filesystem::path root;
    unsigned frame=0,left=0,release=0,buttons=0;
    float x=0,y=0,cx=0,cy=0;
    uint64_t sequence=0,completed=0;
    bool step=false;
    bool command(){
        std::ifstream file(root/"command.txt");
        uint64_t seq;unsigned b,frames;float ax,ay,acx,acy;
        if(!(file>>seq>>b>>ax>>ay>>acx>>acy>>frames) || seq<=sequence ||
           frames<1 || frames>180 || b>0x1fff ||
           !std::isfinite(ax)||!std::isfinite(ay)||!std::isfinite(acx)||!std::isfinite(acy)||
           std::abs(ax)>1||std::abs(ay)>1||std::abs(acx)>1||std::abs(acy)>1)return false;
        sequence=seq;left=frames;buttons=b;x=ax;y=ay;cx=acx;cy=acy;
        return true;
    }
    void observe(){
        char state[8192];melee_sim_agent_observe(state,sizeof(state));
        const auto time=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        std::ofstream file(root/"state.tmp");
        file<<"{\"frame\":"<<frame<<",\"sequence\":"<<sequence<<",\"completed\":"<<completed<<",\"stepped\":"<<(step?"true":"false")<<",\"monotonicMs\":"<<time<<",\"game\":"<<state<<"}\n";
        file.close();
        if(file){std::error_code error;std::filesystem::rename(root/"state.tmp",root/"state.json",error);}
    }
public:
    SimulatorAgent(){
        if(const char* p=std::getenv("MELEE_AGENT_DIR"))root=p;
        if(root.empty())return;
        step=std::getenv("MELEE_AGENT_STEP")!=nullptr;
        // Never replay an input left by a previous app process.
        std::ifstream old(root/"command.txt");old>>sequence;completed=sequence;
    }
    void tick(){
        if(root.empty())return;
        ++frame;
        if(left){if(--left==0){buttons=0;x=y=cx=cy=0;release=3;}}
        else if(release && --release==0)completed=sequence;
        melee_sim_agent_input(buttons,x,y,cx,cy);
        if(frame%6==0 || (!left&&!release))observe();
        if(!left&&!release){
            if(step){
                unsigned waits=0;
                while(!command()){
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    if(++waits%100==0)observe();
                }
            }else command();
            melee_sim_agent_input(buttons,x,y,cx,cy);
        }
    }
};
