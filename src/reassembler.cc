#include "reassembler.hh"
#include "debug.hh"
#include <cstdint>
#include <iterator>
#include <utility>

using namespace std;

void Reassembler::clip_and_merge( uint64_t& first_index, std::string& data)
{
  auto first_unassembled{output_.writer().bytes_pushed()};
  auto capacity{output_.reader().bytes_buffered() + output_.writer().available_capacity()};
  auto upper_bound{output_.reader().bytes_popped() + capacity};
  // clip 左侧越界部分并更新索引
  // 对于每类情况首先考虑最极端的情况, 并直接返回, 后面还有几个类似的例子
  if (first_index + data.size() <= first_unassembled) {
    data.clear();
    return;
  }
  if (first_index < first_unassembled) {
    auto offset{first_unassembled - first_index};
    data.erase(0, offset);
    first_index = first_unassembled;
  }

  // clip 右侧越界部分
  if (first_index >= upper_bound) {
    data.clear();
    return;
  }
  if (first_index + data.size() > upper_bound) {
    auto new_size{upper_bound - first_index};
    data.resize(new_size);
  }
  
  // clip 左侧部分重叠部分
  auto it{buffer_.lower_bound(first_index)};
  // 使用prev必须进行的检查
  if (it != buffer_.begin()) {
    auto prev_it{std::prev(it)};
    auto prev_end{prev_it->first + prev_it->second.size()};
    if (first_index + data.size() <= prev_end) {
      data.clear();
      return;
    }
    if (first_index < prev_end) {
      data.erase(0, prev_end - first_index);
      first_index = prev_end;
    }
  }

  // 迭代增量式依次merge, 并归一化处理clip右侧部分重叠部分
  it = buffer_.lower_bound(first_index);
  // 循环condition一般要从语法本身和语义两方面来考虑
  // insert中还有个类似的例子
  while (it != buffer_.end() && it->first < first_index + data.size()) {
    auto next_end{it->first + it->second.size()};
    if (next_end <= first_index + data.size()) {
      it = buffer_.erase(it);
    } else {
      auto new_size{it->first - first_index};
      data.resize(new_size);
      // 养成思考每个分支都有出循环的可能的习惯
      break;
    }
  }
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  if (is_last_substring) {
    seen_last_ = true;
    end_index_ = first_index + data.size();
  }

  auto check_if_close{[this](){
    if (seen_last_ && end_index_ == output_.writer().bytes_pushed()) {
      output_.writer().close();
    }
  }};

  // 由于map的键是唯一的
  // clip_and_merge调用和if (data.empty())分支判断不能交换顺序
  // 否则当clip_and_merge后的data为空串时
  // buffer_会出现空串占位, 而该索引的有效数据无法插入buffer_, 也无法修改(emplace )
  clip_and_merge(first_index, data);

  if (data.empty()) {
    check_if_close();
    return;
  }

  if (first_index == output_.writer().bytes_pushed()) {
    output_.writer().push(std::move(data));
    
    auto it{buffer_.begin()};
    while (it != buffer_.end() && it->first == output_.writer().bytes_pushed()) {
      output_.writer().push(std::move(it->second));
      it = buffer_.erase(it);
    }
  } else {
    buffer_.emplace(first_index, std::move(data));
  } 

  check_if_close();
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  uint64_t total{0};
  // 结构化绑定
  for (const auto& [seq, data] : buffer_) {
    total += data.size();
  }

  return total;
}
