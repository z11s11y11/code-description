class MemPL_SysConfig {
    public:
      unsigned GPU_device_number;//暂定为1
      unsigned HBM_channel_number;//暂定为80
      unsigned GPU_HBM_BW;//暂定为3TB
      unsigned GPU_HBM_latency;//暂定为100个cc
      unsigned queue_depth; //先设置成无穷大，比如说100000；
      double switch_frec; //沿用core_frec的参数
      double dram_frec;//沿用dram_frec的参数
      double switch_period;//   1/switch_frec
      double dram_period;// 1/dram_frec
}