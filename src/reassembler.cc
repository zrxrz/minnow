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
// data必须是不超过capacity的data
void Reassembler::handle_overloap( uint64_t first_index, std::string& data)
{
  // 如果map为空, 直接返回
  // 因为此函数用于处理重叠问题
  // map为空, 则没有重叠问题, 由调用函数决定是插入map还是push到ByteStream
  if (buffer_.empty()) {
    return;
  }

  auto lower_it{buffer_.lower_bound(first_index)};
  auto upper_it{buffer_.lower_bound(first_index + data.size())};

  if (buffer_.begin() != lower_it) {
    auto lower_prev{std::prev(lower_it)};
    if (first_index <= lower_prev->first + (lower_prev->second).size()) {
      (lower_prev->second).resize(first_index - lower_prev->first);
    }
  }

  // 要对迭代器求prev要求都一样, 它不能是begin()
  if (buffer_.begin() != upper_it) {
    auto upper_prev{std::prev(upper_it)};
    if (first_index + data.size() <= upper_prev->first + (upper_prev->second).size()) {
      data.resize(upper_prev->first - first_index);
    }
    // 避免出现iter指向begin_iter前面而导致的erase UB
    // 倘若buffer_.begin() == upper_it
    // 则一定有lower_it>upper_prev
    // 选择放嵌套if内, 是因为放外面upper_prev出作用域了
    if (lower_it != upper_it) {
      buffer_.erase(lower_it, upper_prev);
    }
  }
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  if (data.empty()) {
    return;
  }

  uint64_t current_capacity_{output_.writer().available_capacity()};
  data = data.substr(0, std::min(current_capacity_ + ack_ - first_index, data.size()));

  // 调用后得到一个可以直接插入map或者push进ByteStream的data
  // 并删除了所有与处理后的data重叠的部分
  handle_overloap(first_index, data);
  if (first_index == ack_) {
    ack_ += data.size();
    output_.writer().push(std::move(data));
    
    auto it{buffer_.begin()};
    while (it != buffer_.end() && it->first == ack_) {
      ack_ += (it->second).size();
      it = buffer_.erase(it);
    }
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
  uint64_t count{0};
  for (const auto& [seq, data] : buffer_) {
    count += data.size();
  }

  return count;
}
