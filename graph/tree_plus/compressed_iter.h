#pragma once

#include "../../common/compression.h"
#include "compressed_nodes_2.h"
//#include "compressed_nodes.h"


// *******************************************
// 压缩列表迭代器
// *******************************************

namespace compressed_lists {

  size_t node_size(uchar* node) {
    if (node) {
      return *((uint16_t*)node);  // 读取前两个字节的数据，表示node大小，大小[8, 16384]
    }
    return static_cast<size_t>(0);
  }

}

namespace compressed_iter {

  using namespace compression;
  using uchar = unsigned char;

  struct read_iter {
    uchar const* node;
    uchar const* start;
    uintV src;
    uintV deg;
    uintV ngh;    // neighbor
    uintV proc;   // 已经处理过的邻居数量
    bool is_valid;

    // uint16_t deg
    // uint16_t size (bytes)
    // uint32_t refct
    // (new): uint32_t last neighbor
    read_iter(uchar const* node, uintV src) : node(node), src(src), deg(0), ngh(0), proc(0) {
      is_valid = (node != nullptr);
      if (is_valid) {
        deg = *((uint16_t*)node);  // 出度
        assert(deg > 0);
        // TODO: 偏移量如何计算？
        start = node + 2*sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t); // 定位到第一个节点
      }
    }

    bool valid() {
      return is_valid;
    }

    inline uintV last_key() {
      uint32_t* last_ngh_ptr = (uint32_t*)(node + 2*sizeof(uint16_t) + sizeof(uint32_t));
      return *last_ngh_ptr;
    }
    
    // 返回下一个邻居
    inline uintV next() {
      if (proc == 0) {
        ngh = read_first_neighbor(start, src);
      } else {
        ngh += read_neighbor(start);
      }
      proc++;
      return ngh;
    }

    inline bool has_next() {
      return proc < deg;
    }
  };

  struct write_iter {
    uchar* node_ptr;       // 写入的节点指针
    size_t offset = 0; 
    size_t proc = 0;
    size_t node_size;      // node_size[8, 16384]

    uintV src; uintV last_ngh;
    uintV deg_ub;          // 度数上限

    // format:
    // uint16_t: deg
    // uint16_t: total size in bytes
    // unint32_t: ref count
    // uint32_t: last neighbor
    write_iter(uintV src, size_t deg_ub) : src(src), deg_ub(deg_ub) {
      proc = 0;
      std::tie(node_ptr, node_size) = compressed_lists::alloc_node(deg_ub*5 + 2 + 2 + 4 + 4); // 2 uint16_t's and 2 uint32_t
      // 这里的5是预期每个gap需要5个字节来编码
      offset += sizeof(uint16_t) + sizeof(uint16_t) + 2*sizeof(uint32_t);
    }

    // 写入ngh
    void compress_next(const uintV& ngh) {
      if (proc == 0) {
        offset = compress_first_neighbor(node_ptr, offset, src, ngh);
      } else {
        uintV difference = ngh - last_ngh;
        offset = compress_neighbor(node_ptr, offset, difference);
      }
      proc++;
      last_ngh = ngh;
    }

    // 全部写完，进行后续记录
    uchar* finish() {
      // write the last neighbor
      if (offset > node_size) {
        cout << "offset = " << offset << " node-size = " << node_size << " src = " << src << " deg = " << deg_ub << " proc = " << proc << endl;
        cout << endl;
        cout << endl;
        assert(offset < node_size);
      }

      uint16_t* deg = (uint16_t*)node_ptr;
      uint16_t* total_size = (uint16_t*)(node_ptr + sizeof(uint16_t));
      uint32_t* ref_ct = (uint32_t*)(node_ptr + 2*sizeof(uint16_t));
      uint32_t* last_ngh_ptr = (uint32_t*)(node_ptr + 2*sizeof(uint16_t) + sizeof(uint32_t));

      *deg = proc;
      *total_size = node_size;
      *ref_ct = 1; 
      *last_ngh_ptr = last_ngh;
      if (proc == 0) {
        compressed_lists::deallocate(node_ptr);
        return nullptr;
      }

      // 内存可以压缩一半
      if (2*offset <= node_size) {
        uchar* old_arr = node_ptr;
        std::tie(node_ptr, node_size) = compressed_lists::alloc_node(offset);
        std::memcpy(node_ptr, old_arr, offset);
        total_size = (uint16_t*)(node_ptr + sizeof(uint16_t));
        compressed_lists::deallocate(old_arr);
      }
      *total_size = offset;
      return node_ptr;
    }
  };

}; // namespace compressed_iter
