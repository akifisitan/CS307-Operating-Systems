#include <semaphore.h>
#include <iostream>
#include <vector>
#include <unistd.h>
#include "Court.h"
using namespace std;

Court* court = nullptr;

void Court::play() {
    sleep(2);
}

void dummy_thread() {
    court->enter();
    court->play();
    court->leave();

}


int main(int argc, char *argv[]){
    int playerNum = atoi(argv[1]);
    int courtSize = atoi(argv[2]);
    int refereePresent = atoi(argv[3]);
    vector<pthread_t> allThreads;
    try {
        court = new Court(courtSize, refereePresent);   
    } catch (const std::exception& e) {
        // Catch all exceptions derived from std::exception
        printf("Exception caught:  %s\n", e.what());
        return 0;
    }

    for(int i=0;i<playerNum;i++){
        pthread_t thread;
        pthread_create(&thread,NULL,(void *(*)(void *))dummy_thread,NULL);
        allThreads.push_back(thread);
    }
    for(int i=0;i<allThreads.size();i++)
        pthread_join(allThreads[i],NULL);
    printf("The Main terminates.\n");
    return 0;
}