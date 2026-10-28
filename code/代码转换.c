// 本文件描述访存请求离开 SM、进入片上网络 INCT 之前的地址译码修改。

//   要做的事情：
//   原设计：80 个 channel，地址只译出 chip ∈ [0, 79]
//   新设计：多于80个channel，如160等，将80个channel定义为一组，一共有dst_n组，一组内的编号为dst_ch，和模拟器之前的chip语义等价

if (!config->is_SST_mode()) {
    config->m_address_mapping.addrdec_tlx(access.get_addr(),&m_raw_addr);
    // 输出 m_raw_addr 的语义变化：
    //   chip        → 现在和 dst_ch 语义等价（节点内部通道号，0~79）
    //   dst_n       → 新增字段，HBM组的编号（0~N-1），我们把80个channel定义为1组
    //   bk/row/col/burst/sub_partition → 节点内部译址结果，语义不变

    // 先不用管，实验先都在!config->is_SST_mode()模式下完成
    m_partition_addr =
        config->m_address_mapping.partition_address(
            access.get_addr()
        );
}

// ---------------------------------------------------------------------------
// addrdec_tlx()
// 完整实现位于：FlashGPU_Sim_Baseline/src/gpgpu-sim/addrdec.cc

// Address Decoder Translation
/*
addrdec_tlx() 的作用是将 GPU 的线性内存地址解码成 DRAM 访问地址，
并确定访存请求对应的 Memory Partition 和 L2 Subpartition。

输入：GPU 线性内存地址 new_addr_type addr，例如 0x2000
输出：addrdec_t
*/

// 我之前的一个疑问https://chatgpt.com/s/t_6ae16c122df08191bc040bc7f4f601cc，已经解决了
void linear_to_raw_address_translation::addrdec_tlx(new_addr_type addr,
                                                    addrdec_t *tlx) const {
  unsigned long long int addr_for_chip, rest_of_addr, rest_of_addr_high_bits;
  if (!gap) {
    // 这里得加入tlx->dst_n的提取

    tlx->chip = addrdec_packbits(addrdec_mask[CHIP], addr, addrdec_mkhigh[CHIP],
                                 addrdec_mklow[CHIP]);
    tlx->bk = addrdec_packbits(addrdec_mask[BK], addr, addrdec_mkhigh[BK],
                               addrdec_mklow[BK]);
    tlx->row = addrdec_packbits(addrdec_mask[ROW], addr, addrdec_mkhigh[ROW],
                                addrdec_mklow[ROW]);
    tlx->col = addrdec_packbits(addrdec_mask[COL], addr, addrdec_mkhigh[COL],
                                addrdec_mklow[COL]);
    tlx->burst = addrdec_packbits(addrdec_mask[BURST], addr,
                                  addrdec_mkhigh[BURST], addrdec_mklow[BURST]);
    rest_of_addr_high_bits =
        (addr >> (ADDR_CHIP_S + (log2channel + log2sub_partition)));

  } else {
    // Split the given address at ADDR_CHIP_S into (MSBs,LSBs)
    // - extract chip address using modulus of MSBs
    // - recreate the rest of the address by stitching the quotient of MSBs and
    // the LSBs
    addr_for_chip = (addr >> ADDR_CHIP_S) % m_n_channel;
    rest_of_addr = ((addr >> ADDR_CHIP_S) / m_n_channel) << ADDR_CHIP_S;
    rest_of_addr_high_bits = ((addr >> ADDR_CHIP_S) / m_n_channel);
    rest_of_addr |= addr & ((1 << ADDR_CHIP_S) - 1);

    // 得加入tlx->dst_n的提取

    tlx->chip = addr_for_chip;
    tlx->bk = addrdec_packbits(addrdec_mask[BK], rest_of_addr,
                               addrdec_mkhigh[BK], addrdec_mklow[BK]);
    tlx->row = addrdec_packbits(addrdec_mask[ROW], rest_of_addr,
                                addrdec_mkhigh[ROW], addrdec_mklow[ROW]);
    tlx->col = addrdec_packbits(addrdec_mask[COL], rest_of_addr,
                                addrdec_mkhigh[COL], addrdec_mklow[COL]);
    tlx->burst = addrdec_packbits(addrdec_mask[BURST], rest_of_addr,
                                  addrdec_mkhigh[BURST], addrdec_mklow[BURST]);
  }
  // 哈希 = 一个"打散器"，把有规律的输入变成均匀、看似随机的输出，用来做分发、查找或校验。
  /*
    memory partition indexing 策略
    switch (memory_partition_indexing) 决定如何把地址映射到 channel × subpartition：
    后面再做重映射的时候，也需要把tlx->dst_n给提取出来
  */
  switch (memory_partition_indexing) {
    case CONSECUTIVE:
    // Do nothing
      break;
    case BITWISE_PERMUTATION: {
    // 用 bitwise_hash_function 对 chip 做异或哈希，打散访问
      assert(!gap);
      tlx->chip = bitwise_hash_function(rest_of_addr_high_bits, tlx->chip, m_n_channel);
      assert(tlx->chip < m_n_channel);
      break;
    }
    case IPOLY: {
    // 用 ipoly_hash_function 做哈希
      // assert(!gap);
      unsigned sub_partition_addr_mask = m_n_sub_partition_in_channel - 1;
      const unsigned decoded_channel = tlx->chip;
      const unsigned decoded_sub_partition_in_channel =tlx->bk & sub_partition_addr_mask;
      unsigned sub_partition =
          decoded_channel * m_n_sub_partition_in_channel +
          decoded_sub_partition_in_channel;
      // Use a lower shift for IPOLY hash input. The original
      // rest_of_addr_high_bits = addr >> (ADDR_CHIP_S + log2channel +
      // log2sub_partition) = addr >> 20, which strips bits that carry
      // stride variation for GEMM workloads (e.g., stride=4096 varies at
      // bit 12), causing severe bank imbalance (12-28 of 128 banks used).
      // Since IPOLY computes its own partition mapping via XOR, feeding it
      // more address bits (from cache-line boundary upward) gives uniform
      // distribution while preserving IPOLY's conflict-free guarantee for
      // power-of-2 strides.
      const unsigned ipoly_shift = 8;  // 256B interleave granularity
      unsigned long long int ipoly_high_bits =
          gap ? rest_of_addr_high_bits : (addr >> ipoly_shift);

      if (gap && ipoly_channel_stable_l2slice) {
        const unsigned slice_hash_seed =
            ((decoded_channel & 0xf) ^ decoded_sub_partition_in_channel);
        const unsigned slice_hash =
            ipoly_hash_function(addr >> ipoly_shift, slice_hash_seed, 16);
        const unsigned sub_partition_in_channel =
            slice_hash % m_n_sub_partition_in_channel;
        sub_partition = decoded_channel * m_n_sub_partition_in_channel +
                        sub_partition_in_channel;
      } else {
        const unsigned total_sub_partitions =
            m_n_channel * m_n_sub_partition_in_channel;
        const unsigned virtual_sub_partitions =
            (gap && ipoly_non_power2_balanced == 2)
                ? 1024
                : nextPowerOf2_m_n_channel * m_n_sub_partition_in_channel;
        sub_partition = ipoly_hash_function(
            ipoly_high_bits, sub_partition, virtual_sub_partitions);

        if (gap) {  // if it is not 2^n partitions, then take modular
          if (ipoly_non_power2_balanced == 1) {
            const unsigned channel = sub_partition % m_n_channel;
            const unsigned sub_partition_in_channel =
                (sub_partition / m_n_channel) % m_n_sub_partition_in_channel;
            sub_partition = channel * m_n_sub_partition_in_channel +
                            sub_partition_in_channel;
          } else if (ipoly_non_power2_balanced == 2) {
            sub_partition =
                (static_cast<unsigned long long>(sub_partition) *
                 total_sub_partitions) /
                virtual_sub_partitions;
          } else {
            sub_partition =
                sub_partition % (m_n_channel * m_n_sub_partition_in_channel);
          }
        }
      }

      tlx->chip = sub_partition / m_n_sub_partition_in_channel;
      tlx->sub_partition = sub_partition;
      assert(tlx->chip < m_n_channel);
      assert(tlx->sub_partition < m_n_channel * m_n_sub_partition_in_channel);
      return;
      break;
    }
    case RANDOM: {
    // 软件哈希表，随机分配，不现实，仅作对比。
      // This is an unrealistic hashing using software hashtable
      // we generate a random set for each memory address and save the value in
      new_addr_type chip_address = (addr >> (ADDR_CHIP_S - log2sub_partition));
      tr1_hash_map<new_addr_type, unsigned>::const_iterator got =
          address_random_interleaving.find(chip_address);
      if (got == address_random_interleaving.end()) {
        unsigned new_chip_id =
            rand() % (m_n_channel * m_n_sub_partition_in_channel);
        address_random_interleaving[chip_address] = new_chip_id;
        tlx->chip = new_chip_id / m_n_sub_partition_in_channel;
        tlx->sub_partition = new_chip_id;
      } else {
        unsigned new_chip_id = got->second;
        tlx->chip = new_chip_id / m_n_sub_partition_in_channel;
        tlx->sub_partition = new_chip_id;
      }

      assert(tlx->chip < m_n_channel);
      assert(tlx->sub_partition < m_n_channel * m_n_sub_partition_in_channel);
      return;
      break;
    }
    case CUSTOM:
    // 用户自己实现
      /* No custom set function implemented */
      // Do you custom index here
      break;
    default:
      assert("\nUndefined set index function.\n" && 0);
      break;
  }

  // combine the chip address and the lower bits of DRAM bank address to form
  // the subpartition ID
  unsigned sub_partition_addr_mask = m_n_sub_partition_in_channel - 1;
  tlx->sub_partition = tlx->chip * m_n_sub_partition_in_channel + (tlx->bk & sub_partition_addr_mask);
}