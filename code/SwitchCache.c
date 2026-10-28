
#ifndef SWITCH_CACHE_H
#define SWITCH_CACHE_H

#include <cstddef>
#include <list>
#include <memory>
#include <queue>

#include "gpu-cache.h"
#include "mem_fetch.h"

class gpgpu_sim;
class mem_fetch_allocator;

// SwitchCache 使用原版 l2_cache。
class SwitchCache {
public:
    SwitchCache(
        cache_config& config,
        mem_fetch_allocator* allocator,
        gpgpu_sim* gpu,
        unsigned dram_queue_size,
        unsigned fill_queue_size,
        unsigned response_queue_size
    );

    ~SwitchCache();

    // 检查 Cache 数据访问端口
    bool can_access() const;

    // 执行 Cache 访问
    // RESERVATION_FAIL 表示上游必须重试
    cache_request_status access(
        mem_fetch* mf,
        unsigned long long cycle
    );

    // 推进缓存流水线
    void cycle(unsigned long long cycle);

    // ========== Cache -> HBM ==========

    bool has_dram_request() const;

    // 先查看请求，HBM 能接收后再 pop
    mem_fetch* top_dram_request() const;

    void pop_dram_request();

    // ========== HBM -> Cache ==========

    bool can_accept_dram_response() const;

    bool push_dram_response(mem_fetch* mf);

    // ========== Cache -> GPU ==========

    bool has_response() const;

    mem_fetch* pop_response();

private:
    // 对接 l2_cache 的下层内存接口
    class SwitchCachePort : public mem_fetch_interface {
    public:
        explicit SwitchCachePort(SwitchCache* owner)
            : m_owner(owner) {}

        bool full(unsigned size, bool write) const override;

        void push(mem_fetch* mf) override;

    private:
        SwitchCache* m_owner;
    };

    // 被引用的配置必须比 l2_cache 活得更久
    cache_config& m_config;

    // l2_cache 的 Miss 请求出口
    SwitchCachePort m_memport;

    // 原版 FlashGPU-Sim L2 Cache
    std::unique_ptr<l2_cache> m_cache;

    // Cache 发往 HBM 的请求
    std::queue<mem_fetch*> m_dram_request_queue;

    // HBM 返回、等待 Cache Fill 的请求
    std::queue<mem_fetch*> m_dram_response_queue;

    // Cache 完成后返回上游的请求
    std::queue<mem_fetch*> m_response_queue;

    // 有界队列容量
    unsigned m_dram_queue_size;
    unsigned m_fill_queue_size;
    unsigned m_response_queue_size;
};

#endif
