#include "tcp_connection.hh"
#include "tcp_helpers/tcp_state.hh"
#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().buffer_size(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    if(!active()) return;
    _last_segment_received = 0;

    if(seg.header().rst){
        unclean_shutdown();
        return;
    }
    bool _need_sent_empty = seg.length_in_sequence_space();
    
    _receiver.segment_received(seg);

    if(seg.header().ack){
        _sender.ack_received(seg.header().ackno,seg.header().win);
        if(!_sender.segments_out().empty()) _need_sent_empty  =false;

    }

    // if(seg.header().seqno == _receiver.ackno() && seg.length_in_sequence_space() && _sender.segments_out().empty()){
    //     _sender.send_empty_segment();
    //     push_segment_out();
    //     return;
    // }
    // if(_receiver._out_of_window(seg) && _sender.segments_out().empty()){
    //     _sender.send_empty_segment();
    //     push_segment_out();
    //     return;
    // }
    
    if(TCPState::state_summary(_receiver) == TCPReceiverStateSummary::SYN_RECV && \
        TCPState::state_summary(_sender) == TCPSenderStateSummary::CLOSED){
            connect();
            return;
        }

    if(_receiver.stream_out().eof() && !_sender.snetfin()){_linger_after_streams_finish = false;}

    // LAST_ACK --> CLOSED
    if(_receiver.stream_out().eof() && _sender.stream_in().eof() && _sender.next_seqno_absolute() \
        ==_sender.stream_in().bytes_written()+2 && !bytes_in_flight() && !_linger_after_streams_finish){
        _active = false;
    }

    if(_need_sent_empty) _sender.send_empty_segment();
    push_segment_out();
}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    size_t len =  _sender.stream_in().write(data);
    _sender.fill_window();
    push_segment_out();
    return len;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if(!active()) return;
    _last_segment_received += ms_since_last_tick;
    if(state() == TCPState::State::TIME_WAIT && _last_segment_received >= 10*_sender.init_retransmission_time() \
    && _linger_after_streams_finish){
        _linger_after_streams_finish = false;
        _active = false;
        return;
    }
    _sender.tick(ms_since_last_tick);
    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS){
        _sender.send_rst_segment();
        unclean_shutdown();
    }
    push_segment_out();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    push_segment_out();
}

void TCPConnection::connect() {
    _sender.fill_window();
    push_segment_out();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            _sender.send_rst_segment();
            unclean_shutdown();
            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::push_segment_out(){
    // if(!active()) return;
    while(!_sender.segments_out().empty()){
        auto& seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if(_receiver.ackno().has_value()){
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = min(numeric_limits<uint16_t>::max(),static_cast<uint16_t>(_receiver.window_size()));
        }
        _segments_out.push(seg);
    }
}

void TCPConnection::unclean_shutdown(){
      _receiver.stream_out().set_error();
      _sender.stream_in().set_error();
      _active = false;
}

