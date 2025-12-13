#include "byte_stream.hh"
#include "debug.hh"
#include <cstdint>

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity )
  , error_( false )
  , buffer_( "" )
  , bytes_pushed_( 0 )
  , writer_closed_( false )
  , bytes_popped_( 0 )
{}

// Push data to stream, but only as much as available capacity allows.
void Writer::push( string data )
{
  if ( data.empty() ) {
    return;
  }

  const uint64_t bytes_to_write { std::min( data.size(), available_capacity() ) };

  buffer_.append( data.substr( 0, bytes_to_write ) );
  bytes_pushed_ += bytes_to_write;
}

// Signal that the stream has reached its ending. Nothing more will be written.
void Writer::close()
{
  writer_closed_ = true;
}

// Has the stream been closed?
bool Writer::is_closed() const
{
  return writer_closed_;
}

// How many bytes can be pushed to the stream right now?
uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer_.size(); // Your code here.
}

// Total number of bytes cumulatively pushed to the stream
uint64_t Writer::bytes_pushed() const
{
  return bytes_pushed_; // Your code here.
}

// Peek at the next bytes in the buffer -- ideally as many as possible.
// It's not required to return a string_view of the *whole* buffer, but
// if the peeked string_view is only one byte at a time, it will probably force
// the caller to do a lot of extra work.
string_view Reader::peek() const
{
  return buffer_;
}

// Remove `len` bytes from the buffer.
void Reader::pop( uint64_t len )
{
  const uint64_t bytes_to_pop { min( len, buffer_.size() ) };
  buffer_.erase( 0, bytes_to_pop );
  bytes_popped_ += bytes_to_pop;
}

// Is the stream finished (closed and fully popped)?
bool Reader::is_finished() const
{
  return writer_closed_ && !bytes_buffered();
}

// Number of bytes currently buffered (pushed and not popped)
uint64_t Reader::bytes_buffered() const
{
  return buffer_.size();
}

// Total number of bytes cumulatively popped from stream
uint64_t Reader::bytes_popped() const
{
  return bytes_popped_;
}
