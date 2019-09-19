#include <map>
#include <cassert>
#include <cstdlib>
#include <stdint.h>
#include <sys/types.h>
#include <string>
#include <cstddef>
#include <iostream>
#include <vector>

#define MAX_SKIPLIST_HIGH 13
// 2^31-1
#define MAX_INT32 2147483647L 
#define SEED 16807

typedef int Key;

struct SkipNode {
  Key key;

  SkipNode(const Key& k, int h) : key(k), high_(h) {}
  
  SkipNode* get_next(int n) {
    assert(n >= 0 && n < high_);
    return next_[n];
  }
  
  void set_next(int n, SkipNode* node) {
    assert(n >= 0 && n < high_);
    next_[n] = node;
  }

  int get_high() { return high_; }
private:
  int high_;
  SkipNode* next_[1];
};

struct SkipList {

  struct Random {
private:
    uint32_t seed_;

public:
    Random(uint32_t s) : seed_(s & 0x7fffffffu) {}

    int32_t get_next() {
      // We are computing
      // seed_ = (seed_ * SEED) % MAX_INT32,    where MAX_INT32 = 2^31-1
      //
      // seed_ must not be zero or MAX_INT32, or else all subsequent computed values
      // will be zero or M respectively.  For all other values, seed_ will end
      // up cycling through every number in [1,M-1]
      uint64_t product = seed_ * SEED;

      // Compute (product % MAX_INT32) using the fact that ((x << 31) % MAX_INT32) == x.
      seed_ = static_cast<uint32_t>((product >> 31) + (product & MAX_INT32));
      // The first reduction may overflow by 1 bit, so we may need to
      // repeat. mod == MAX_INT32 is not possible; using > allows the faster
      // sign-bit-based test.
      if (seed_ > MAX_INT32) {
        seed_ -= MAX_INT32;
      }
      return seed_;
    }
  };

  SkipNode* find_great_or_equal(const Key& key, SkipNode** prev);
  SkipNode* find_less_than(const Key& key);
  int randon_heigth();
  int get_max_high() { return high_; }
  void insert(const Key& key);
  void delete_key(const Key& key);
  //SkipNode* find(const Key& key);
  SkipNode* malloc_node(const Key& key, int level);
  int key_is_after_node(const Key& key, SkipNode* node) {
    if (node == nullptr || key <= node->key) {
      return 0;
    }
    return 1; 
  }

  SkipList() : high_(1), rand_(0xdeadbeef) {
    head_ = malloc_node(0, MAX_SKIPLIST_HIGH);
    for (int i = 0; i < MAX_SKIPLIST_HIGH; i++) {
      head_->set_next(i, nullptr);
    }
  }
  
private:
  SkipNode* head_;
  Random rand_;
  int high_;
  
  void set_max_high(int h) {
    //assert(high_ < h);
    high_ = h;
  }
};

int SkipList::randon_heigth() {
  const uint32_t kBranching = 4;
  int height = 1;
  while (height < MAX_SKIPLIST_HIGH && ((rand_.get_next() % kBranching) == 0)) {
    height++;
  }
  assert(height > 0 && height <= MAX_SKIPLIST_HIGH);
  return height;
}

SkipNode* SkipList::find_great_or_equal(const Key& key, SkipNode** prev) {
  SkipNode* node = head_;
  int level = get_max_high() - 1;
  while (true) {
    SkipNode* next = node->get_next(level);
    // compare less than key
    if (key_is_after_node(key, next)) {
    node = next;
    } else {
      if (prev) {
        prev[level] = node;
      }
      if (level == 0) {
        // greate or equal to key
        return next;
      } else {
        level--;
      }
    }
  }
}

SkipNode* SkipList::find_less_than(const Key& key) {
  SkipNode* node = head_;
  int level = get_max_high() - 1;
  while(true) {
    SkipNode* next = node->get_next(level);
    if (next != nullptr && key > next->key) {
      node = next;
    } else {
      if (level == 0) {
        // less than to key
        return node;
      } else {
        level--;
      }
    }
  }  
}

void SkipList::insert(const Key& key) {
  if (key == head_->key) {
    return;
  }
  SkipNode* prev[MAX_SKIPLIST_HIGH];
  SkipNode* node = find_great_or_equal(key, prev);

  if (node != nullptr && key == node->key) {
    std::cout << __func__ << " key is already existed, key:" << key << std::endl;
    return;
  }
  
  int level = randon_heigth();
   std::cout << __func__ << " key:" << key << " , high: " << level
             << " , old high:" << get_max_high() << std::endl;
  if (level > get_max_high()) {
    for (int l = get_max_high(); l < level; l++) {
      prev[l] = head_;
    }
    set_max_high(level);
  }

  SkipNode* new_node = malloc_node(key, level);
  for (int l =0; l < level; l++) {
    new_node->set_next(l, prev[l]->get_next(l));
    std::cout << __func__ << " begin level:" << l << " insert:" << key
              << ", prev:" << prev[l]->key << " , next:";
    if (prev[l]->get_next(l) == nullptr)
      std::cout << "null";
    else
      std::cout << prev[l]->get_next(l)->key;
    std::cout << std::endl;
    prev[l]->set_next(l, new_node);
  }
}

SkipNode* SkipList::malloc_node(const Key& key, int level) {
  //const int align = (sizeof(void*) > 8) ? sizeof(void*) : 8;
  //assert(align & (align - 1) == 0);
  int size = sizeof(SkipNode) * level;
  char* const node_memory = new char[size];
  return new (node_memory) SkipNode(key, level);
}

void SkipList::delete_key(const Key& key) {
  if (key == head_->key) {
    return;
  }
  SkipNode* prev[MAX_SKIPLIST_HIGH];
  SkipNode* node = find_great_or_equal(key, prev);

  if (!node || node->key != key) {
    std::cout << __func__ << " not found key :" << key << std::endl;
    return;
  }
  int new_skiplist_high = get_max_high();
  std::cout << __func__ << " skiplist high :" << new_skiplist_high << std::endl;
  int node_high = node->get_high();
  if (new_skiplist_high == node_high) {
    for (int i = new_skiplist_high - 1; i > 0; i--) {
      if (prev[i] != head_ || node->get_next(i) != nullptr)
        break;
      new_skiplist_high = i;
    }
  }
  
  if (new_skiplist_high < get_max_high()) {
    std::cout << __func__ << " new skiplist high:" << new_skiplist_high << std::endl;
    set_max_high(new_skiplist_high);
  }
  
  for (int i = 0; i < node_high; i++) {
    prev[i]->set_next(i, node->get_next(i));
    std::cout << __func__ << " begin level:" << i << " delete:" << key
              << ", prev:" << prev[i]->key << " , next:";
    if (node->get_next(i) == nullptr)
      std::cout << "null";
    else
      std::cout << node->get_next(i)->key;
    std::cout << std::endl;
  }
  std::cout << __func__ << " delete key:" << key << std::endl;
  delete node;
}

int main()
{
  SkipList slist;
  std::vector<int> keys;
  keys.reserve(30);
  
  for (int i = 0; i < 30; i++) {
    keys.push_back(rand() % 100);
    slist.insert(keys.back());
  }

  for (int i = 0; i < 100; i++) {
    slist.delete_key(keys[rand() % 30]);
  }
  
  return 0;
}
