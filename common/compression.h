#pragma once


// *******************************************
// Implements byte-compression primitives
// 差分编码
// *******************************************

#define LAST_BIT_SET(b) (b & (0x80))  // 最高位为1表示有后续字节
#define EDGE_SIZE_PER_BYTE 7  //第一个字节只有6位有效，后续所有都是7位

typedef unsigned char uchar;

using namespace std;
namespace compression {

  // 读取第一个邻居
  inline uintV read_first_neighbor(uchar const* &start, const uintV& src) {
    uchar fb = *start++;
    uintV edgeRead = (fb & 0x3f);  // 第一个字节
    // 读取后续字节
    if (LAST_BIT_SET(fb)) {
      int shiftAmount = 6;
      while (1) {
        uchar b = *start;
        edgeRead |= ((b & 0x7f) << shiftAmount);
        start++;
        if (LAST_BIT_SET(b))
          shiftAmount += EDGE_SIZE_PER_BYTE;
        else
           break;
      }
    }
    return (fb & 0x40) ? src - edgeRead : src + edgeRead; 
  }

  // 返回编码值
  inline uintV read_neighbor(uchar const* &start) {
    uintV edgeRead = 0;
    int shiftAmount = 0;

    while (1) {
      uchar b = *start++;
      edgeRead += ((b & 0x7f) << shiftAmount);
      if (LAST_BIT_SET(b))
        shiftAmount += EDGE_SIZE_PER_BYTE;
      else
        break;
    }
    return edgeRead;
  }

  /*
    Compresses the first edge, writing target-source and a sign bit.
    压缩源节点到目标节点的gap值，返回新的offset
  */
  long compress_first_neighbor(uchar *start, size_t offset, long source, long target) {
    long diff = target - source;  // 计算gap值
    long preCompress = diff;
    uchar firstByte = 0;
    uintE toCompress = std::abs(preCompress); // absolute value
    firstByte = toCompress & 0x3f; // 0011|1111
    if (preCompress < 0) {
      firstByte |= 0x40;  // 第7位表示符号位
    }
    // 检查是否需要更多字节
    toCompress = toCompress >> 6;
    if (toCompress > 0) {
      firstByte |= 0x80; // 最高位置1,表示有后续字节
    }
    start[offset] = firstByte; // write first
    offset++;

    uchar curByte = toCompress & 0x7f;
    // 剩余部分继续编码
    while ((curByte > 0) || (toCompress > 0)) {
      uchar toWrite = curByte;
      toCompress = toCompress >> 7;
      // Check to see if there's any bits left to represent
      curByte = toCompress & 0x7f;
      if (toCompress > 0) {
        toWrite |= 0x80;
      }
      start[offset] = toWrite;
      offset++;
    }
    return offset;
  }

  // value表示gap值
  inline size_t compress_neighbor(uchar* start, size_t cur_offset, uintV value) {
    uchar cur_byte = value & 0x7f;
    while ((cur_byte > 0) || (value > 0)) {
      uchar to_write = cur_byte;
      value = value >> 7;
      // Check to see if there's any bits left to represent
      cur_byte = value & 0x7f;
      if (value > 0) {
        to_write |= 0x80;
      }
      start[cur_offset] = to_write;
      cur_offset++;
    }
    return cur_offset;
  }

} // namespace compression
