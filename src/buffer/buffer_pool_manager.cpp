//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/buffer_pool_manager.h"
#include <list>
#include <unordered_map>

namespace bustub {

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager, LogManager *log_manager)
    : pool_size_(pool_size), disk_manager_(disk_manager), log_manager_(log_manager) {
  // We allocate a consecutive memory space for the buffer pool.
  pages_ = new Page[pool_size_];
  replacer_ = new LRUReplacer(pool_size);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
}

BufferPoolManager::~BufferPoolManager() {
  delete[] pages_;
  delete replacer_;
}

Page *BufferPoolManager::FetchPageImpl(page_id_t page_id) {
  // 1.     Search the page table for the requested page (P).
  // 1.1    If P exists, pin it and return it immediately.
  // 1.2    If P does not exist, find a replacement page (R) from either the free list or the replacer.
  //        Note that pages are always found from the free list first.
  // 2.     If R is dirty, write it back to the disk.
  // 3.     Delete R from the page table and insert P.
  // 4.     Update P's metadata, read in the page content from disk, and then return a pointer to P.

  frame_id_t frame_id;
  if (this->GetFrameIdByPageId(page_id, &frame_id)) {
    this->replacer_->Pin(frame_id);
    return this->GetPages() + frame_id;
  }
  Page *page;
  if (!this->free_list_.empty()) {  // free frame
    frame_id = this->free_list_.front();
    this->free_list_.pop_front();
  } else {
    if (!this->replacer_->Victim(&frame_id)) {
      return nullptr;
    }
    auto *old_page = this->GetPages()+ frame_id;
    page_id_t old_page_id;
    auto pos = std::find_if(this->page_table_.begin(), this->page_table_.end(),
                            [&frame_id](auto pair) { return pair.second == frame_id; });
    old_page_id = pos->first;
    if (old_page->IsDirty()) {  // victim a dirty page, write back
      this->disk_manager_->WritePage(old_page_id, old_page->GetData());
    }

    // remove old_page_id -> frame_id relation
    this->page_table_.erase(pos);
    old_page->ResetMemory();
  }
  page = this->GetPages() + frame_id;

  // read data to new page
  this->disk_manager_->ReadPage(page_id, page->GetData());
  // add new relation page_id -> frame_id
  this->page_table_[page_id] = frame_id;

  this->replacer_->Pin(frame_id);
  if (page->pin_count_ <= 0) {
    page->pin_count_ = 1;
  }

  return page;
}

bool BufferPoolManager::UnpinPageImpl(page_id_t page_id, bool is_dirty) {
  frame_id_t frame_id;
  if (this->GetFrameIdByPageId(page_id, &frame_id)) {
    auto page = this->GetPages() + frame_id;
    int old_pin_count = page->pin_count_;

    if (old_pin_count == 1) {
      page->pin_count_ = 0;
      this->replacer_->Unpin(frame_id);
    } else if (old_pin_count > 1) {
      page->pin_count_ -= 1;
    } else {
      // nop
    }

    page->is_dirty_ = is_dirty;

    if (old_pin_count <= 0) {
      return false;
    } else {
      return true;
    }
  }

  return false;
}

bool BufferPoolManager::GetFrameIdByPageId(page_id_t page_id, frame_id_t *frame_id) {
  auto pos = this->page_table_.find(page_id);
  if (pos != this->page_table_.end()) {  // find in pages
    *frame_id = pos->second;
    return true;
  } else {
    return false;
  }
}

bool BufferPoolManager::FlushPageImpl(page_id_t page_id) {
  // Make sure you call DiskManager::WritePage!
  frame_id_t frame_id;
  if (this->GetFrameIdByPageId(page_id, &frame_id)) {
    auto page = this->GetPages() + frame_id;
    this->disk_manager_->WritePage(page_id, page->GetData());
    return true;
  }
  return false;
}

Page *BufferPoolManager::NewPageImpl(page_id_t *page_id) {
  // 0.   Make sure you call DiskManager::AllocatePage!
  // 1.   If all the pages in the buffer pool are pinned, return nullptr.
  // 2.   Pick a victim page P from either the free list or the replacer. Always pick from the free list first.
  // 3.   Update P's metadata, zero out memory and add P to the page table.
  // 4.   Set the page ID output parameter. Return a pointer to P.

  if (this->page_table_.size() >= pool_size_) {
    // page_id_t unpin_page_id;
    auto pos = std::find_if(this->page_table_.begin(), this->page_table_.end(),
                            [this](auto pair)
                            { return (this->GetPages()+pair.second)->GetPinCount() <= 0; });
    if (pos == this->page_table_.end()) {
      return nullptr;
    }
  }
  
  *page_id = this->disk_manager_->AllocatePage();

  // a new page_id, and a new/old frame_id

  frame_id_t frame_id;
  page_id_t old_page_id;
  if (!this->free_list_.empty()) {
    frame_id = this->free_list_.front();
    this->free_list_.pop_front();
  } else {
    this->replacer_->Victim(&frame_id);  // must be true
    auto pos = std::find_if(this->page_table_.begin(), this->page_table_.end(),
                            [&frame_id](auto pair) { return pair.second == frame_id; });
    old_page_id = pos->first;
  }
  auto page = this->GetPages() + frame_id;
  if (page->IsDirty()) {
    this->disk_manager_->WritePage(old_page_id, page->GetData());
    page->ResetMemory();
  }

  this->page_table_[*page_id] = frame_id;
  page->pin_count_ += 1;

  return page;
}

bool BufferPoolManager::DeletePageImpl(page_id_t page_id) {
  // 0.   Make sure you call DiskManager::DeallocatePage!
  // 1.   Search the page table for the requested page (P).
  // 1.   If P does not exist, return true.
  // 2.   If P exists, but has a non-zero pin-count, return false. Someone is using the page.
  // 3.   Otherwise, P can be deleted. Remove P from the page table, reset its metadata and return it to the free list.
  this->disk_manager_->DeallocatePage(page_id);
  frame_id_t frame_id;

  if (this->GetFrameIdByPageId(page_id, &frame_id)) {
    auto page = this->GetPages() + frame_id;
    if (page->GetPinCount() > 0) {
      return false;
    }
    auto pos = this->page_table_.find(page_id);
    this->page_table_.erase(pos);
    page->ResetMemory();
    this->free_list_.push_back(frame_id);
  }
  return true;
}

void BufferPoolManager::FlushAllPagesImpl() {
  // You can do it!
  for (auto it = this->page_table_.begin(); it != this->page_table_.end(); ++it) {
    this->FlushPage(it->first);
  }
}

}  // namespace bustub
