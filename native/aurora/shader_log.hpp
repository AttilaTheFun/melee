#pragma once
// The renderer only enqueues bounded records. Formatting, rotation, and disk I/O
// happen on this worker, independently of the game and shader compiler threads.
#include "lib/gfx/pipeline_cache.hpp"
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <thread>

class MeleeShaderLog {
    struct Record { aurora::gfx::PipelineEvent event; uint64_t time; uint64_t context; };
    std::array<Record, 2048> records{};
    size_t head=0, count=0;
    std::mutex mutex;
    std::condition_variable wake;
    std::thread worker;
    bool stop=false;
    std::filesystem::path directory;
public:
    std::atomic<uint64_t> dropped{0}, discoveries{0};
    std::atomic<uint64_t> context{UINT64_MAX};
    std::atomic<bool> failed{false};
    void start(const std::filesystem::path& path) {
        directory=path;
        worker=std::thread([this]{write();});
    }
    void record(const aurora::gfx::PipelineEvent& event) {
        if(event.kind==aurora::gfx::PipelineEvent::Discovered)++discoveries;
        const auto now=std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        {
            std::lock_guard lock(mutex);
            if(count==records.size()){++dropped;return;}
            records[(head+count)%records.size()]={event,static_cast<uint64_t>(now),context.load()};
            ++count;
        }
        wake.notify_one();
    }
    ~MeleeShaderLog() {
        {std::lock_guard lock(mutex);stop=true;}wake.notify_one();
        if(worker.joinable())worker.join();
    }
private:
    void write() noexcept {
        try {
            const auto current=directory/"shader-events.jsonl";
            const auto previous=directory/"shader-events-previous.jsonl";
            std::filesystem::create_directories(directory);
            uint64_t size=std::filesystem::exists(current)?std::filesystem::file_size(current):0;
            std::ofstream file(current,std::ios::app);
            if(!file)throw std::runtime_error("shader log open");
            const auto session=std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            uint64_t reportedDrops=0;
            for(;;){
                Record r;
                {
                    std::unique_lock lock(mutex);
                    wake.wait(lock,[this]{return stop||count;});
                    if(!count&&stop)return;
                    r=records[head];head=(head+1)%records.size();--count;
                }
                if(size>=2'000'000){
                    file.close();std::filesystem::remove(previous);
                    std::filesystem::rename(current,previous);
                    file.open(current,std::ios::trunc);size=0;
                }
                const auto& e=r.event;
                const char* names[]={"discovered","compiled","waited","warmup_complete"};
                file<<"{\"session\":"<<session<<",\"uptimeNs\":"<<r.time
                    <<",\"event\":\""<<names[e.kind]<<"\",\"hash\":\""<<std::hex<<e.hash<<std::dec
                    <<"\",\"durationNs\":"<<e.durationNs;
                // UINT32_MAX denotes a render-worker wait without a safely readable frame number.
                if(e.frame!=UINT32_MAX)file<<",\"firstFrame\":"<<e.frame;
                if(e.kind==aurora::gfx::PipelineEvent::Compiled)
                    file<<",\"fromCatalog\":"<<(e.cached?"true":"false");
                if(r.context!=UINT64_MAX)file<<",\"mode\":"<<((r.context>>32)&65535)
                    <<",\"scene\":"<<((r.context>>16)&65535)<<",\"stage\":"<<(r.context&65535);
                const auto lost=dropped.load();
                file<<",\"droppedEvents\":"<<(lost-reportedDrops)<<"}\n";
                reportedDrops=lost;
                file.flush();
                if(!file)throw std::runtime_error("shader log write");
                size=static_cast<uint64_t>(file.tellp());
            }
        }catch(...){failed.store(true);}
    }
};
