//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_replacer.h
//
// Identification: src/include/buffer/lru_replacer.h
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <list>
#include <mutex>  // NOLINT
#include <vector>
#include <list>

#include "buffer/replacer.h"
#include "common/config.h"

namespace bustub {

/**
 * LRUReplacer implements the lru replacement policy, which approximates the Least Recently Used policy.
 * The size of the LRUReplacer is the same as buffer pool
 * since it contains placeholders for all of the frames in the BufferPoolManager.
 * However, not all the frames are considered as in the LRUReplacer.
 * The LRUReplacer is initialized to have no frame in it.
 * Then, only the newly unpinned ones will be considered in the LRUReplacer
 */
class LRUReplacer : public Replacer {
 public:
  /**
   * Create a new LRUReplacer.
   * @param num_pages the maximum number of pages the LRUReplacer will be required to store
   */
  explicit LRUReplacer(size_t num_pages);

  std::list<frame_id_t> pages;
  size_t num_pages;

  /**
   * Destroys the LRUReplacer.
   */
  ~LRUReplacer() override;

  /**
   * Remove the object that was accessed the least recently compared
   * to all the elements being tracked by the Replacer,
   * store its contents in the output parameter and return True.
   * If the Replacer is empty return False
   */
  bool Victim(frame_id_t *frame_id) override;

  /**
   * This method should be called after a page is pinned to a frame in the BufferPoolManager.
   * It should remove the frame containing the pinned page from the LRUReplacer
   */
  void Pin(frame_id_t frame_id) override;

  /**
   * This method should be called when the pin_count of a page becomes 0.
   * This method should add the frame containing the unpinned page to the LRUReplacer
   */
  void Unpin(frame_id_t frame_id) override;

  // This method returns the number of frames that are currently in the LRUReplacer
  size_t Size() override;

 private:
  // TODO(student): implement me!
};

}  // namespace bustub
