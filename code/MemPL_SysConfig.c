class MemPL_SysConfig {
    public:
      unsigned GPU_device_number;
      unsigned HBM_channel_number;
      unsigned GPU_HBM_BW;
      unsigned GPU_HBM_latency;
      unsigned queue_depth; //先设置成无穷大，比如说100000；
      double switch_frec; //沿用core_frec的参数
      double dram_frec;//沿用dram_frec的参数
      double switch_period;
      double dram_period;
}