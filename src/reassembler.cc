#include "reassembler.hh"
#include "debug.hh"
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <utility>

using namespace std;

// std::string&
void Reassembler::insert_helper( uint64_t first_index, std::string& data)
{
  auto begin_iter{buffer_.lower_bound(first_index)};
  auto end_iter{buffer_.lower_bound(first_index + data.size())};

  auto iter{buffer_.erase(begin_iter, std::prev(end_iter))};
  if (first_index + data.size() >= iter->first + (iter->second).size()) {
    buffer_.erase(iter);
  }
  data = data.substr(0, iter->first - first_index);
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  uint64_t current_capacity_{output_.writer().available_capacity()};
  data = data.substr(0, std::min(current_capacity_ + ack_ - first_index, data.size()));

  if (first_index == ack_) {
    insert_helper(first_index, data);
    ack_ = first_index + data.size();
    output_.writer().push(std::move(data));
  } else if (first_index > ack_) {
    insert_helper(first_index, data);
    buffer_.emplace(first_index, data);
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
