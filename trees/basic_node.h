#pragma once
#include "../pbbslib/list_allocator.h"
#include <typeinfo>       // operator typeid

using node_size_t = unsigned int;
//using node_size_t = size_t;

// 基础树节点
template<class balance, class _ET>
struct basic_node {
  using ET = _ET;

  struct node : balance {
    node* lc;  node* rc;
    ET entry;             // 存储节点的实际数据（类型为 ET）
    node_size_t s;        // 节点的大小，表示以该节点为根的子树的节点数目
    node_size_t ref_cnt;  // 引用计数
  };

  using allocator = list_allocator<node>;   // 内存池分配器

  static node_size_t size(node* a) {
    return (a == NULL) ? 0 : a->s;
  }

  static void update(node* a) {
    a->s = size(a->lc) + size(a->rc) + 1;
  }

  static node* make_node(ET e) {
    node *o = allocator::alloc();
    o->ref_cnt = 1;
    //o->entry = e;
    pbbs::assign_uninitialized(o->entry,e);
    return o;
  }

  static node* single(ET e) {
    node* r = make_node(e);
    r->lc = r->rc = NULL; r->s = 1;
    return r;
  }

  static void free_node(node* a) {
    (a->entry).~ET();
    allocator::free(a);
  }

  static node* empty() {return NULL;}
  inline static ET& get_entry(node *a) {return a->entry;}
  inline static ET* get_entry_p(node *a) {return &(a->entry);}
  static void set_entry(node *a, ET e) {a->entry = e;}
  static node* left(node a) {return a.lc;}
  static node* right(node* a) {return a.rc;}
};
