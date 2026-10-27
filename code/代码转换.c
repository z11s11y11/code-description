// FlashGPU_Sim_Baseline/
// └── src/
//     └── gpgpu-sim/
//         └── mem_fetch.cc

if (!config->is_SST_mode()) {
    config->m_address_mapping.addrdec_tlx(
        access.get_addr(),
        &m_raw_addr
        // 这里其实是将addr转换成了带有信息的m_raw_addr
    );

    m_partition_addr =
        config->m_address_mapping.partition_address(
            access.get_addr()
        );
}

// addrdec_tlx()函数
// addrdec_tlx() 的完整实现位于：

// FlashGPU_Sim_Baseline/src/gpgpu-sim/addrdec.cc

void linear_to_raw_address_translation::addrdec_tlx(
    new_addr_type addr,
    addrdec_t *tlx
) const {

    // 1. 根据地址提取 DRAM 字段
    if (!gap) {
        // 从原始地址提取
        // chip, bank, row, column, burst
    } else {
        // 处理非 2 的幂 Channel 数量
        // 重新构造分区内部地址
        // 再提取 bank, row, column, burst
    }

    // 2. 根据配置执行分区映射
    switch (memory_partition_indexing) {

        case CONSECUTIVE:
            // 保留原始通道映射
            break;

        case BITWISE_PERMUTATION:
            // 使用位运算 Hash 修改 chip
            break;

        case IPOLY:
            // 通过 IPOLY Hash 计算 sub_partition
            // 并根据 sub_partition 更新 chip
            return;

        case RANDOM:
            // 使用持久化随机地址映射
            return;

        case CUSTOM:
            // 自定义映射
            break;

        default:
            assert(false);
    }

    // 3. 常规策略计算 L2 Sub-partition
    unsigned mask = m_n_sub_partition_in_channel - 1;

    tlx->sub_partition =
        tlx->chip * m_n_sub_partition_in_channel +
        (tlx->bk & mask);
}

现在其实是需要将dst_n和dst_ch也加入到解析的过程之中。