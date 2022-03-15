#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) :_capacity(capacity) {}

size_t ByteStream::write(const string &data) {
    if(endinput) return 0;
    size_t n = data.size();
    n = min(n,remaining_capacity());
    for(size_t i=0;i<n;i++) datastream.push_back(data[i]);
    nwrite +=n;
    return n;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    //size_t now_nums = buffer_size();
    string res;
    //if(len > now_nums) return res;
    auto tmp = datastream.begin();
    for(size_t i=0;i<len;i++,tmp++){
        // res+= (*tmp);
        res.push_back(*tmp);
    }
    return res;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    // size_t now_nums = buffer_size();
    // if(len > now_nums) return ;
    for(size_t i=0;i<len;i++) datastream.pop_front();
    nread +=len;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string res = peek_output(len);
    pop_output(len);
    //nread +=len;
    return res;
}

void ByteStream::end_input() { endinput = true;}

bool ByteStream::input_ended() const { return endinput; }

size_t ByteStream::buffer_size() const { return nwrite - nread; }

bool ByteStream::buffer_empty() const { return datastream.empty(); }

bool ByteStream::eof() const { return input_ended() && buffer_empty(); }

size_t ByteStream::bytes_written() const { return nwrite; }

size_t ByteStream::bytes_read() const { return nread; }

size_t ByteStream::remaining_capacity() const { return _capacity - buffer_size(); }
