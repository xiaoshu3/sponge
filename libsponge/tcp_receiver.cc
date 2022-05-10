#include "tcp_receiver.hh"
#include "wrapping_integers.hh"
// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if(!_getsyn && !seg.header().syn) return;
    if(!_getsyn && seg.header().syn){
        _isn =  seg.header().seqno;
        _getsyn  = 1;
    }
    uint64_t index = unwrap(seg.header().seqno,_isn,_reassembler._checkpoint()) + (seg.header().syn? 1:0);
    _reassembler.push_substring(static_cast<string>(seg.payload().str()),index-1,seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(!_getsyn) return {};
    uint64_t n = _reassembler._checkpoint() + stream_out().input_ended();
    return wrap(n+1,_isn);
}

size_t TCPReceiver::window_size() const{ return _reassembler.window_size();}
