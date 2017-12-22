#ifndef LINKED_LIST_HPP
#define LINKED_LIST_HPP

#include <stddef.h>

template <class T>
struct Node {
  Node<T> *prev, *next;
  T value;
  
  Node(T value);
};

template <class T>
class LinkedList {
public:
  size_t length;
  Node<T> *head, *tail;
  LinkedList();
  void push_back(T value);
  void push_back(Node<T> *node);
  void erase(Node<T> *node);
};

template <class T>
Node<T>::Node(T value) {
  this->next = nullptr;
  this->prev = nullptr;
  this->value = value;
}

template <class T>
LinkedList<T>::LinkedList() {
  this->length = 0;
  this->head = nullptr;
  this->tail = nullptr;
}

template <class T>
void LinkedList<T>::push_back(T value) {
  Node<T> *node = new Node<T>(value);
  this->push_back(node);
}

template <class T>
void LinkedList<T>::push_back(Node<T> *node) {
  if (this->length == 0) {
    this->tail = node;
    this->head = node;
  } else {
    node->prev = this->tail;
    node->next = nullptr;
    this->tail->next = node;
    this->tail = node;
  }
  this->length += 1;
}

template <class T>
void LinkedList<T>::erase(Node<T> *node) {
  if (node->prev != nullptr) {
    node->prev->next = node->next;
  } else if (node->prev == nullptr) {
    this->head = node->next;
  }
  
  if (node->next != nullptr) {
    node->next->prev = node->prev;
  } else if (node->next == nullptr) {
    this->tail = node->prev;
  }
  
  delete node;
  this->length -= 1;
}

#endif