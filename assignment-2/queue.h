#ifndef QUEUE_H
#define QUEUE_H

#include "pthread.h"
#include <iostream>
#include <stdexcept>

using namespace std;

template<typename T>
class Node {
public:
  T value;
  Node* next;

  Node() : value(T()), next(nullptr) {}
  Node(T val) : value(val), next(nullptr) {}
};

/*
  Michael and Scott Concurrent Queue
*/
template<typename T>
class Queue {

private:
  Node<T>* head;
  Node<T>* tail;
  pthread_mutex_t head_lock;
  pthread_mutex_t tail_lock;

public:
  Queue() {
    Node<T>* tmp = new Node<T>();
    tmp->next = nullptr;
    head = tail = tmp;
    pthread_mutex_init(&head_lock, nullptr);
    pthread_mutex_init(&tail_lock, nullptr);
  }

  void enqueue(T item) {
    Node<T>* tmp = new Node<T>();
    tmp->value = item;
    tmp->next = nullptr;
    pthread_mutex_lock(&tail_lock);
    tail->next = tmp;
    tail = tmp;
    pthread_mutex_unlock(&tail_lock);
  };

  T dequeue() {
    pthread_mutex_lock(&head_lock);
    Node<T>* tmp = head;
    Node<T>* new_head = tmp->next;
    if (new_head == nullptr) {
      pthread_mutex_unlock(&head_lock);
      throw out_of_range("Attempt to dequeue from an empty queue");
    }
    T value = new_head->value;
    head = new_head;
    pthread_mutex_unlock(&head_lock);
    free(tmp);
    return value;
  };

  bool isEmpty() {
    return head == tail;
  }

  void print() {
    Node<T>* iter = head->next;
    if (iter == nullptr) {
      cout << "Empty\n";
      return;
    }
    while (iter != nullptr) {
      if (iter->next != nullptr) {
        cout << iter->value << " ";
      }
      else {
        cout << iter->value << "\n";
      }
      iter = iter->next;
    }
  }


};

#endif