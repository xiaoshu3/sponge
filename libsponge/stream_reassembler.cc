#include "stream_reassembler.hh"
#include <iostream>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity){}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t start = index,end;
    string substring = move(data);
    if(index < pushed_pos){
        if(data.size() + index <= pushed_pos) return;
        substring = substring.substr(pushed_pos - index);
        start  = pushed_pos;
    }
    if(eof) get_eof = index + data.size();
    size_t len = substring.size();
    
    if(len > remain_capacity()){
        if(start == pushed_pos){
            pushed_pos += min(substring.size(),_output.remaining_capacity());
            _output.write(substring);
        }
        else substring = substring.substr(0,remain_capacity());
    }
    //if(substring == "") return;
    end = start + substring.size();

    //cerr<<start<<" "<<end <<" " <<substring<<endl;

    insert_TmpStore(start,end,substring);
    auto it = TmpStore.begin();
    while(it != TmpStore.end() && it->start <= pushed_pos){
        if(it->end <= pushed_pos){
            TmpStore.erase(it++);
            continue;
        }
        
        size_t maxsent = _output.remaining_capacity();
        if(maxsent >= it->end - pushed_pos){
            _output.write(it->substring.substr(pushed_pos - it->start));
            pushed_pos = it->end;
        }
        else{
            _output.write(it->substring.substr(pushed_pos - it->start,pushed_pos - it->start+ maxsent));
            pushed_pos+= maxsent;
            TmpStore.insert({pushed_pos,it->end-pushed_pos,it->substring.substr(pushed_pos - it->start)});
        }
        
        
        TmpStore.erase(it++);
    }
    if(get_eof  == pushed_pos ) _output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const { 
    auto it  = TmpStore.begin();
    size_t last = 0,res = 0;
    if(it == TmpStore.end()) return 0;
    else{
        res += it->end - it->start;
        last = it->end;
        ++it;
    }
    while(it != TmpStore.end()){
        if(it->start < last){
            if(it->end <= last){
                it++;
                continue;
            }
            else res += (it->end - last);
        }
        else res +=(it->end - it->start);
        
        last = it->end;
        it++;
    }
    return res;

}

bool StreamReassembler::empty() const { return unassembled_bytes() == 0; }

void StreamReassembler::insert_TmpStore(size_t start,size_t end,string& substring){
    if(end <= start) return;
    TmpStore.insert({start,end,move(substring)});
    // auto it  = TmpStore.begin();
    // while(it != TmpStore.end()){
    //     if(start > it->end || end < it->start) ++it;
    //     else break;
    // }
    // if(it == TmpStore.end()){
    //     TmpStore.insert({start,end,move(substring)});
    //     return;
    // }
    // if(start <= it->start && end<= it->end) return;
    // else if(it->start < start && it->end < end){
    //     TmpStore.erase(it);
    //     TmpStore.insert({start,end,move(substring)});
    //     return;
    // }
    // else{
    //     if(start < it->start){
    //         substring.append(it->substring.substr(end-it->start));
    //         end = it->end;
    //         TmpStore.erase(it);
    //         TmpStore.insert({start,end,move(substring)});
    //         return;
    //     }
    //     else{
    //         string s(move(it->substring));
    //         s.append(substring.substr(it->end - start));
    //         start  = it->start;
    //         TmpStore.erase(it);
    //         TmpStore.insert({start,end,move(substring)});
    //     }
    // }
}

