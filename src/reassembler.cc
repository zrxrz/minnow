#include "reassembler.hh"
#include "debug.hh"
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <utility>

using namespace std;

// 删除所有和data存在完全重叠的节点
// 设和data存在部分重叠的节点为N, 不重叠的部分为NP
// 将NP append到data, 并删除N
// 需要注意的是data的类型是std::string&
void Reassembler::insert_helper( uint64_t first_index, std::string& data)
{
  auto begin_iter{buffer_.lower_bound(first_index)};
  auto end_iter{buffer_.lower_bound(first_index + data.size())};

  // 避免出现iter指向begin_iter前面而导致的erase UB
  if (begin_iter == end_iter) {
    return;
  }

  auto iter{buffer_.erase(begin_iter, std::prev(end_iter))};
  if (first_index + data.size() < iter->first + (iter->second).size()) {
    data = data.substr(0, iter->first - first_index);
    // 假设data是当前的next bytes in the stream
    // 由于iter指向的子串和现在的data是连续的, 
    // 即data一旦push到ByteStream, iter指向的first就是ack_
    // iter指向的second就是 the next bytes in the stream
    // 这样可以简化first_index == ack_时的调用
    data += iter->second;
  }  
  buffer_.erase(iter);
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  uint64_t current_capacity_{output_.writer().available_capacity()};
  data = data.substr(0, std::min(current_capacity_ + ack_ - first_index, data.size()));

  // 调用后得到一个可以直接插入map或者push进ByteStream的data
  // 并删除了所有与处理后的data重叠的部分
  insert_helper(first_index, data);
  if (first_index == ack_) {
    ack_ = first_index + data.size();
    output_.writer().push(std::move(data));
  } else if (first_index > ack_) {
    buffer_.emplace(first_index, data);
  } else {
    data = data.substr(ack_ - first_index);
    insert(ack_,std::move(data), is_last_substring );
  }

  if (is_last_substring) {
    output_.writer().close();
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  debug( "unimplemented count_bytes_pending() called" );
  return {};
}
