#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    //if(_geteof >= 0 ) return;
    if(eof) _geteof = index + data.size();
    if(_geteof == _pushedpos) _output.end_input();
    string substring(data);
    
    if(index <= _pushedpos){
        if(index + data.size() <= _pushedpos) return;
        else{
            substring = move(data.substr(_pushedpos - index));
            size_t writelen = _output.write(substring);
            _pushedpos += writelen;
            if(writelen <  substring.size()){
                substring = move(substring.substr(writelen));
                insert_set(_pushedpos, substring);
            }
        }
    }else{
        insert_set(index,substring);
    }

    auto it =  _setStore.begin();
    while(it != _setStore.end() && it->_start <= _pushedpos){
        if(it->_end <= _pushedpos) _setStore.erase(it++);
        else{
            _pushedpos += _output.write(it->_substring.substr(_pushedpos - it->_start));
            if(_pushedpos < it->_end) break;
            else _setStore.erase(it++);
        }
    }

    if(_geteof == _pushedpos) _output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const {
    size_t res = 0,last;
    auto it  = _setStore.begin();
    if(it!= _setStore.end()) last = it->_start;
    while(it != _setStore.end()){
        if(last < it->_end){
            res+= (it->_end - max(last,it->_start));
            last = it->_end;
        } 
        it++;
    }
    return res;
}

bool StreamReassembler::empty() const { return _setStore.empty(); }

void StreamReassembler::insert_set(size_t start,string& s){
    if(!s.size()) return;
    size_t len = remain_capacity();
    if(len >= s.size()) _setStore.insert({start,start+s.size(),move(s)});
    else{
        _setStore.insert({start,start+len,move(s.substr(0,len))});
    }

}

size_t StreamReassembler::remain_capacity()const{
    size_t reman = _output.remaining_capacity(),unassembled = unassembled_bytes();
    // return _output.remaining_capacity() - unassembled_bytes();
    if (reman <= unassembled) return 0;
    return reman - unassembled;
}