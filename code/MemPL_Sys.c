class MemPL_Sys {
    public:
        void init(const MemPL_SysConfig *cfg);
        void cycle(); //驱动所有的dram_channel
    private:
        std::vector<dram_t*> m_channels;
        // m_channels的数量为cfg->HBM_channel_number
}