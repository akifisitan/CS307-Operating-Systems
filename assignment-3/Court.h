#ifndef COURT_H
#define COURT_H

#include "pthread.h"
#include "semaphore.h"
#include <exception>
#include "stdlib.h"
#include "stdio.h"

using namespace std;


class Court {

private:

  int numPlayers;             // number of players inside the court
  int numWaiting;             // number of players waiting to enter the court
  int numPlayersNeeded;       // number of players (excluding referee) needed to start a match
  int refereeRequired;        // if a referee will take part in the court
  bool matchOngoing;          // status of the match
  pthread_t refereeId;        // id of the referee taking part in the court 
  sem_t lockNumPlayers;       // binary semaphore (lock) for atomic modification of numPlayers
  sem_t lockNumWaiting;       // binary semaphore (lock) for atomic modification of numWaiting
  sem_t lockEnter;            // binary semaphore (lock) for synchronizing enter method body
  sem_t lockMatchStatus;      // binary semaphore (lock) for atomic read/write operations on match status value
  sem_t waitMatchEnd;         // semaphore for players waiting to enter the court
  pthread_barrier_t barrier;  // barrier for synchronizing print statements

public:

  Court(int courtSize, int refereePresent) {
    if (courtSize <= 0) {
      throw invalid_argument("An error occurred.");
    }
    if (refereePresent != 0 && refereePresent != 1) {
      throw invalid_argument("An error occurred.");
    }
    numPlayers = 0;
    numWaiting = 0;
    numPlayersNeeded = courtSize;
    refereeRequired = refereePresent;
    matchOngoing = false;
    refereeId = 0;
    sem_init(&lockNumPlayers, 0, 1);
    sem_init(&lockEnter, 0, 1);
    sem_init(&lockNumWaiting, 0, 1);
    sem_init(&lockMatchStatus, 0, 1);
    sem_init(&waitMatchEnd, 0, 0);
    int barrierThreshold = refereeRequired ? numPlayersNeeded + 1 : numPlayersNeeded;
    pthread_barrier_init(&barrier, nullptr, barrierThreshold);
  }


  /*
    Threads call this to attempt to enter the court if it is not already full
  */
  void enter() {
    pthread_t tid = pthread_self();
    printf("Thread ID: %lu, I have arrived at the court.\n", (unsigned long)tid);

    sem_wait(&lockEnter); // Grab enter lock to synchronize enter method body
    sem_wait(&lockMatchStatus); // Grab match status lock to read status value

    // Loop instead of single if check to allow for re-checking match status after waking up, not a busy waiting loop
    while (matchOngoing) {
      sem_post(&lockEnter); // Release enter lock before going to sleep to prevent deadlock 
      sem_post(&lockMatchStatus); // Release match status lock, already read the value 
      sem_wait(&lockNumWaiting);
      numWaiting++; // Atomically increment waiting player count
      sem_post(&lockNumWaiting);
      sem_wait(&waitMatchEnd); // Wait on the semaphore until the last player signals
      sem_wait(&lockNumWaiting);
      numWaiting--; // Atomically decrement waiting player count after waking up
      sem_post(&lockNumWaiting);
      sem_wait(&lockEnter); // Re-grab enter lock to synchronize enter method body after waking up
      sem_wait(&lockMatchStatus); // Re-grab match status lock to re-check status value after waking up
    }
    sem_post(&lockMatchStatus); // Release match status lock, already read the value

    sem_wait(&lockNumPlayers);
    numPlayers++; // Atomically increment player count
    sem_post(&lockNumPlayers);

    // If a referee is required and there are already enough players, make this player the referee and start the match
    // Else if a referee is not required and there are already enough players, make this player start the match
    // Else just print line and return from enter
    if (refereeRequired && numPlayers == (numPlayersNeeded + 1)) {
      sem_wait(&lockMatchStatus); // Grab match status lock to set status value
      refereeId = tid;
      matchOngoing = true;
      printf("Thread ID: %lu, There are enough players, starting a match.\n", (unsigned long)tid);
      sem_post(&lockMatchStatus); // Release match status lock after setting its value
    }
    else if (!refereeRequired && numPlayers == numPlayersNeeded) {
      sem_wait(&lockMatchStatus); // Grab match status lock to set status value
      matchOngoing = true;
      printf("Thread ID: %lu, There are enough players, starting a match.\n", (unsigned long)tid);
      sem_post(&lockMatchStatus); // Release match status lock after setting its value
    }
    else {
      printf("Thread ID: %lu, There are only %d players, passing some time.\n", (unsigned long)tid, numPlayers);
    }

    sem_post(&lockEnter); // Release enter lock, method complete
  }

  /*
    Will be implemented by the testers
  */
  void play();


  /*
    Threads call this right after play() to leave the court
  */
  void leave() {
    pthread_t tid = pthread_self();

    sem_wait(&lockMatchStatus); // Grab match status lock to read status value
    // Match hasn't started by the time play() is complete, just leave
    if (!matchOngoing) {
      printf("Thread ID: %lu, I was not able to find a match and I have to leave.\n", (unsigned long)tid);
      sem_wait(&lockNumPlayers);
      numPlayers--; // Atomically decrement player count
      sem_post(&lockNumPlayers);
      sem_post(&lockMatchStatus); // Release match status lock
      return;
    }
    sem_post(&lockMatchStatus); // Release match status lock, already read the value

    // Match was formed, wait for the referee or the starting player to have printed "starting a match"
    // This means just wait for everyone, makes up for the worst case that the referee or the starting player comes last
    pthread_barrier_wait(&barrier);
    // The referee or starting player should now have printed the "starting a match" line 

    // If referee is required, wait for the referee to print its line
    // Else just leave in any order
    if (refereeRequired) {
      if (pthread_equal(refereeId, tid)) {
        printf("Thread ID: %lu, I am the referee and now, match is over. I am leaving.\n", (unsigned long)tid);
        pthread_barrier_wait(&barrier); // will print its line before the barrier is destroyed
      }
      else {
        pthread_barrier_wait(&barrier); // will not print their line until the barrier is destroyed
        printf("Thread ID: %lu, I am a player and now, I am leaving.\n", (unsigned long)tid);
      }
    }
    else {
      printf("Thread ID: %lu, I am a player and now, I am leaving.\n", (unsigned long)tid);
    }

    sem_wait(&lockNumPlayers); // Grab num player lock to atomically set and read numPlayers
    numPlayers--;
    // The last player to leave ends the game and wakes up the players waiting inside the enter() method 
    if (numPlayers == 0) {
      sem_post(&lockNumPlayers); // Release num player lock, already set and read the value
      sem_wait(&lockMatchStatus); // Grab match status lock to atomically read and set status value
      printf("Thread ID: %lu, everybody left, letting any waiting people know.\n", (unsigned long)tid);
      sem_wait(&lockNumWaiting); // Grab num waiting lock to atomically read numWaiting
      // Wake up players waiting on the semaphore
      for (int i = 0; i < numWaiting; i++) {
        sem_post(&waitMatchEnd);
      }
      sem_post(&lockNumWaiting); // Release num waiting lock
      matchOngoing = false;
      sem_post(&lockMatchStatus); // Release match status lock
    }
    else {
      sem_post(&lockNumPlayers); // Release num player lock, already set and read the value
    }
  }

};

#endif