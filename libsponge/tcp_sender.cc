#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) 
    , t(retx_timeout){}

uint64_t TCPSender::bytes_in_flight() const {
    return _bufferStore.size() + (_next_seqno >0 && _acknoed_num == 0) \
    + (_sentFIN && _acknoed_num < _next_seqno);
}

void TCPSender::fill_window() {
    if(!_sent_window){_next_window_size = _remain_window_size = 1;}
    if(_sentFIN) return;
    if(_next_window_size && _remain_window_size == 0) return;
    if(_acknoed_num + _next_window_size < _next_seqno ) return;
    else if(_acknoed_num + _next_window_size  == _next_seqno && _next_window_size) return;

    size_t len = (_acknoed_num + _next_window_size) - _next_seqno;
    if (len == 0){
        t.set_not_back_off();
        len = 1;
        _remain_window_size = 1;
    }

    len -= (_next_seqno == 0);
    string s = move(_stream.read(len));

    // TCPSegment tmp{};
    // tmp.header().syn = (_next_seqno == 0);
    // tmp.header().fin = _stream.eof();
    // // tmp.header().seqno = _isn + static_cast<uint32_t>(_next_seqno);
    // tmp.header().seqno = wrap(_next_seqno,_isn);
    // tmp.payload() = Buffer(move(s));
    
    // size_t length = tmp.length_in_sequence_space();
    // if(length == 0) return;

    // _next_seqno += length;
    // // _bufferStore.append(BufferList(tmp.payload()));
    // _bufferStore.append(tmp.payload());
    // segments_out().push(tmp);
    // t.start();
    if(_next_seqno > 0 && !_stream.eof() && !s.size()) return;
    len = s.size();
    if(len == 0 ) len = 1;

    for (unsigned int i = 0; i  < len;) {
        size_t send_size = min(TCPConfig::MAX_PAYLOAD_SIZE,len -i );
        TCPSegment tmp;
        if(i == 0 && _next_seqno == 0 ){
            tmp.header().syn = true;
            send_size -= 1;
            if(_remain_window_size) _remain_window_size-=1;
        }
        tmp.header().seqno = wrap(_next_seqno,_isn);
        if(s.size()){
            tmp.payload() = Buffer(move(s.substr(i,send_size)));
            if(_remain_window_size >= send_size) _remain_window_size -= send_size;
        }
        if(i+ TCPConfig::MAX_PAYLOAD_SIZE >= len && _stream.eof()){
            if(_remain_window_size){
                _remain_window_size -= 1;
                tmp.header().fin = true;
                _sentFIN = true;
            }
        } 
        size_t length = tmp.length_in_sequence_space();
        if(length == 0) break;
        _next_seqno += length;
        _bufferStore.append(tmp.payload());
        segments_out().push(tmp);
        i+=send_size;
    }
    t.start();
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t checkpoint = unwrap(ackno,_isn,_acknoed_num);
    if(checkpoint > _next_seqno || checkpoint < _acknoed_num) return;

    _sent_window = true;
    size_t n = checkpoint - _acknoed_num;
    _next_window_size = window_size;
    _remain_window_size = window_size;
    
    if(n > 0 ){
        n -= (_next_seqno >0 && _acknoed_num == 0);
        if(_sentFIN && checkpoint == _next_seqno) n-=1;

        t.resert();
        if(bytes_in_flight()) t.start();
        if(_sentFIN && _bufferStore.buffers().size() == 1 && n == _bufferStore.buffers().front().size()){
            if(checkpoint < _next_seqno) return;
        }
         _acknoed_num = checkpoint;
        _bufferStore.remove_prefix(n);
    }
    // fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if(!t._isRunning()) return;
    t.add(ms_since_last_tick);
    if(t.isExpire()){
        if(bytes_in_flight()){
            t.expire();
            retransmit();
        }
        else t.resert();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const {
    return t._retransmissions_times;
}

void TCPSender::send_empty_segment() {
    TCPSegment tmp{};
    tmp.header().seqno = _isn + static_cast<uint32_t>(_next_seqno);
    segments_out().push(tmp);
}

void TCPSender::retransmit(){
    TCPSegment tmp;
    tmp.header().syn = (_acknoed_num == 0);
    tmp.header().fin = _sentFIN && _bufferStore.buffers().size() == 1;
    tmp.header().seqno = wrap(_acknoed_num,_isn);
    tmp.payload() = _bufferStore.buffers().front();
    segments_out().push(tmp);
    t.start();
}