#include "tcp_receiver.hh"
#include "byte_stream.hh"
#include "debug.hh"
#include "tcp_receiver_message.hh"
#include "wrapping_integers.hh"
#include <algorithm>
#include <climits>
#include <cstdint>
#include <optional>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  if ( message.RST ) {
    reader().set_error();
  }
  if ( message.SYN ) {
    zero_point_ = message.seqno;
  }
  if ( !zero_point_.has_value() ) {
    return;
  }
  /*
  Internet: seqno(Wrap32)
  TCPReciver: seqno → abs_seq(unwrap) → stream_idx
  Reassembler: stream_idx
  */
  // writer().bytes_pushed()是期望的the next stream_idx
  // 而ckpt是估计当前绝对位置的bs_seq, 需要加上SYN所占的空字节
  auto ckpt { writer().bytes_pushed() + 1 };
  auto abs_seq { message.seqno.unwrap( zero_point_.value(), ckpt ) };
  // 若不是和SYN一起的data, stream_idx = abs_seq - 1
  // 否则, stream_idx = abs_seq(initial)
  auto first_index { abs_seq - 1 + ( message.SYN ? 1 : 0 ) };
  reassembler_.insert( first_index, message.payload, message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  TCPReceiverMessage msg;
  if ( !zero_point_.has_value() ) {
    msg.ackno = std::nullopt;
  } else {
    auto abs_ackno { writer().bytes_pushed() + 1 + ( writer().is_closed() ? 1 : 0 ) };
    msg.ackno = Wrap32::wrap( abs_ackno, zero_point_.value() );
  }

  auto capacity { writer().available_capacity() };
  msg.window_size = std::min( capacity, static_cast<uint64_t>( UINT16_MAX ) );

  if ( reader().has_error() ) {
    msg.RST = true;
  }

  return msg;
}
