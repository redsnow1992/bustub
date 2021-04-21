//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_replacer.cpp
//
// Identification: src/buffer/lru_replacer.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_replacer.h"
#include "algorithm"

namespace bustub {

LRUReplacer::LRUReplacer(size_t num_pages) {
    this->num_pages = num_pages;
}

LRUReplacer::~LRUReplacer() = default;

bool LRUReplacer::Victim(frame_id_t *frame_id) {
    if (this->pages.empty()) {
        return false;
    }

    *frame_id = this->pages.front();
    this->pages.pop_front();
    return true;
}

void LRUReplacer::Pin(frame_id_t frame_id) {
    std::list<frame_id_t>::iterator pos;
    pos = std::find(this->pages.begin(), this->pages.end(), frame_id);

    if (pos != this->pages.end()) {
        this->pages.erase(pos);
    }
}

void LRUReplacer::Unpin(frame_id_t frame_id) {
    // not found
    if (std::find(this->pages.begin(), this->pages.end(), frame_id) == this->pages.end()) {
        if (this->pages.size() >= this->num_pages) {
           this->pages.pop_front();
        }
        this->pages.push_back(frame_id);
    }
}

size_t LRUReplacer::Size() { return this->pages.size(); }

}  // namespace bustub
