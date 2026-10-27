// MemPL_Sys类的定义如下

class MemPL_Sys {
public:
    // 初始化内存系统
    void init(const MemPL_SysConfig* cfg);

    // 推进内存系统一个 DRAM 时钟周期
    void cycle();

    // 判断指定目标通道能否接收请求
    bool can_accept(unsigned dst_n,
                    unsigned dst_ch) const;

    // 接收来自GPU的memory partition unit的访存请求
    bool push_request(mem_fetch* mf,
                      unsigned dst_n,
                      unsigned dst_ch);

    // 检查是否存在已完成的访存请求
    bool has_response() const;

    // 获取已完成的访存请求
    mem_fetch* pop_response();

private:
    MemPL_SysConfig m_config;

    // DRAM 通道
    std::vector<dram_t*> m_channels;

    // 已完成请求的队列
    std::queue<mem_fetch*> m_response_queue;

    // 将 GPU ID 和 HBM 通道号映射到通道索引
    unsigned get_channel_index(
        unsigned dst_n,
        unsigned dst_ch) const;
};
