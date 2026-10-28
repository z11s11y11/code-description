/*
访存流程图如下：
                    MemPL_Sys
                        |
                  MemPL_Switch
                        |
              Request / Input Queue
                        |
                 SwitchCache
                   /      \
                HIT       MISS
                 |          |
           Response      MSHR
             Queue         |
                           |
                      DRAM Queue
                           |
                          HBM
                           |
                      Fill Cache
                           |
                     Response Queue
                           |
                          GPU
*/


class MemPL_Switch {
public:
    void init(const MemPL_SysConfig* cfg);

    bool can_accept() const;

    bool push_request(mem_fetch* mf,
                      unsigned dst_n,
                      unsigned dst_ch);

    void cycle();

    // SwitchCache miss 后发往 HBM 的请求
    bool has_dram_request() const;

    mem_fetch* pop_dram_request();

    unsigned get_request_channel() const;

    // 接收 HBM 返回的数据
    void push_dram_response(mem_fetch* mf);

    // 返回 GPU 的请求
    bool has_response() const;

    mem_fetch* pop_response();

private:
    // SwitchCache
    std::unique_ptr<SwitchCache> m_cache;

    // GPU 到 Switch 的输入队列
    std::queue<SwitchRequest> m_request_queue;

    // SwitchCache miss 后的 HBM 请求队列
    std::queue<SwitchRequest> m_dram_request_queue;

    // HBM 返回后的响应队列
    std::queue<SwitchResponse> m_response_queue;

    // 路由和仲裁状态
    unsigned m_rr_next;
};