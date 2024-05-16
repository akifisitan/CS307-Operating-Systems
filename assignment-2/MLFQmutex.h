#ifndef MLFQMUTEX_H
#define MLFQMUTEX_H

#include "park.h"
#include "queue.h"
#include <iostream>
#include <string>
#include <atomic>
#include "pthread.h"
#include <unordered_map>
#include <vector>
#include <chrono>

using namespace std;

class MLFQMutex {

private:
  int flag; // Lock flag 
  atomic_flag guard = ATOMIC_FLAG_INIT; // Atomic flag to synchronize lock() and unlock() bodies
  double qVal; // Quantum (time slice) value 
  vector<Queue<pthread_t>*> queueList; // List of queues from priority 0 (max priority) to numPriorityLevels (min priority)
  Garage* garage; // Associated object to call park, unpark and setPark to put threads to sleep
  unordered_map<pthread_t, int> threadLevelMap; // Maps thread_id to priorityLevel
  chrono::high_resolution_clock::time_point ts_start; // Stores start timestamp 
  chrono::high_resolution_clock::time_point ts_end; // Stores end timestamp


public:
  MLFQMutex(int numPLevels, double quantumValue) : qVal(quantumValue), garage(new Garage()), flag(0) {
    queueList = vector<Queue<pthread_t>*>(numPLevels);
    for (int i = 0; i < queueList.size(); i++) {
      Queue<pthread_t>* q = new Queue<pthread_t>();
      queueList[i] = q;
    }
  }

  void lock() {
    while (guard.test_and_set(memory_order_acquire))
      ; // acquire guard lock by spinning

    if (flag == 0) {
      flag = 1; // lock is acquired
      ts_start = chrono::high_resolution_clock::now(); // Take timestamp after acquiring lock
      guard.clear(memory_order_release);
    }
    else {
      pthread_t t_id = pthread_self();
      int priorityLevel = 0;
      // If thread id is not in the map, add it to the map with value 0
      // Else, grab the existing value
      if (threadLevelMap.find(t_id) == threadLevelMap.end()) {
        threadLevelMap[t_id] = priorityLevel;
      }
      else {
        priorityLevel = threadLevelMap[t_id];
      }
      // Add t_id to the queue located at the priority level
      cout << "Adding thread with ID: " << t_id << " to level " << priorityLevel << endl;
      cout.flush();
      queueList[priorityLevel]->enqueue(t_id);
      // Signal that this thread will park to prevent signal loss
      garage->setPark();
      // Release guard lock
      guard.clear(memory_order_release);
      // Park this thread
      garage->park();
      // Take timestamp right after being dequeued from the sleep queue and unparked (woken up)
      ts_start = chrono::high_resolution_clock::now();
    }
  }

  void unlock() {
    while (guard.test_and_set(memory_order_acquire))
      ; // acquire guard lock by spinning

    ts_end = chrono::high_resolution_clock::now(); // Take timestamp right before giving back lock 
    double exec_time = chrono::duration_cast<chrono::duration<double>>(ts_end - ts_start).count(); // Calculate critical section execution time
    pthread_t t_id = pthread_self();

    int previousPriorityLevel = 0;

    // Update previous priority level with stored value if it exists
    bool tidInMap = threadLevelMap.find(t_id) != threadLevelMap.end();
    if (tidInMap) {
      previousPriorityLevel = threadLevelMap[t_id];
    }

    // If critical section execution time is bigger than quantum value, 
    // increase priorityLevel value (which decreases actual priority) by floor(execution time/quantum value)
    // however, the new priority level cannot be higher than the lowest available priority level 
    if (exec_time > qVal) {
      int newPriorityLevel = previousPriorityLevel + (int)(exec_time / qVal);
      int lowestPriorityLevel = queueList.size() - 1;
      // Update priority level for next runs
      threadLevelMap[t_id] = newPriorityLevel > lowestPriorityLevel ? lowestPriorityLevel : newPriorityLevel;
    }

    // Find next sleeping thread to run
    pthread_t next;
    bool noSleepingThreads = true;
    // Go through the queues by priority (0, 1, ..., pMin)
    for (int i = 0; i < queueList.size(); i++) {
      // Check if queue with priority i is empty
      // If it is, continue checking
      // If it is not, dequeue from the queue, found the next thread to run
      if (!queueList[i]->isEmpty()) {
        next = queueList[i]->dequeue();
        noSleepingThreads = false;
        break;
      }
    }

    if (noSleepingThreads) {
      flag = 0; // let go of the lock, no sleeping threads 
    }
    else {
      garage->unpark(next); // wake up the next thread in the queue and keep holding the lock for it
    }

    guard.clear(memory_order_release);
  }

  /*
    Calls print() on each queue
  */
  void print() {
    cout << "Waiting threads:\n";
    for (int i = 0; i < queueList.size(); i++) {
      cout << "Level " << i << ":";
      queueList[i]->print();
    }
  }


};


#endif