/*

  NAME:         AVERY MITCHELL
  CLASS:        CS 3800
  INSTRUCTOR:   JOSHUA WILKERSON

*/

#include "process.h"
#include "ioModule.h"
#include "processMgmt.h"

#include <chrono> // for sleep
#include <thread> // for sleep

int main(int argc, char* argv[])
{
    // vector of processes, processes will appear here when they are created by
    // the ProcessMgmt object (in other words, automatically at the appropriate time)
    list<Process> processList;
    
    // list of iterators that point to ready processes in the processList
    list<list<Process>::iterator> readyList;

    // vector of iterators that point to blocked processes in the processList
    vector <list<Process>::iterator> blockedList;

    // iterator that points to the currently running process
    list<Process>::iterator curRun;

    // bool that keeps track if a process is currently running
    bool bcurRun = 0;

    // this will orchestrate process creation in our system, it will add processes to 
    // processList when they are created and ready to be run/managed
    ProcessManagement processMgmt(processList);

    // this is where interrupts will appear when the ioModule detects that an IO operation is complete
    // look at what process is here and move them to the ready state and then remove them
    list<IOInterrupt> interrupts;   

    // this manages io operations and will raise interrupts to signal io completion
    IOModule ioModule(interrupts);  

    // this tells if all the processes in the processList are in the done state
    bool allDone = 0;

    // this tells if all the processes in the processList are in the ready state
    bool allReady = 0;

    long time = 1;
    long sleepDuration = 50;
    string file;
    stringstream ss;
    enum stepActionEnum {noAct, admitNewProc, handleInterrupt, beginRun, continueRun, ioRequest, complete} stepAction;

    switch(argc)
    {
        case 1:
            file = "./procList.txt";  // default input file
            break;
        case 2:
            file = argv[1];         // file given from command line
            break;
        case 3:
            file = argv[1];         // file given
            ss.str(argv[2]);        // sleep duration given
            ss >> sleepDuration;
            break;
        default:
            cerr << "incorrect number of command line arguments" << endl;
            cout << "usage: " << argv[0] << " [file] [sleepDuration]" << endl;
            return 1;
            break;
    }

    processMgmt.readProcessFile(file);
    
    // Iterators for the process list
    list<Process>::iterator itr = processList.begin();
    list<Process>::iterator end = processList.begin();

    time = 0;
    //processorAvailable = true;

    while((processMgmt.moreProcessesComing() || allDone != 1)&&time<500)
    {
        //Update our current time step
        ++time;

        //let new processes in if there are any
        processMgmt.activateProcesses(time);

        //update the status for any active IO requests
        ioModule.ioProcessing(time);

        stepAction = noAct;

        //This loop is responsible for adding processes to my ready list 
        //and selecting the process to run (before IO request)
        int tmp = 0;
        if((processMgmt.moreProcessesComing() == 1) || (allReady == 0)){
          itr++;

          //adds to readyList if it is in the ready state
          if(itr->state == 0){
            readyList.push_back(itr);
            itr->state = ready;
          }

          //adds the first newArrival to the readyList
          else if(itr->state == 3){
            readyList.push_back(itr);
            itr->state = ready;
            tmp++;
            stepAction = admitNewProc;
          }
          end = itr;

          //Checks if all the processes are in the ready state
          list<Process>::iterator red = processList.begin();
          for(unsigned long i=0; i<processList.size(); i++){
            allReady = 1;
            if(red->state != ready){
              allReady = 0;
            }
            red++;
          }
        }

        if(processMgmt.moreProcessesComing()){
          stepAction = admitNewProc;
        }

        //Once all the processes are admitted and put into the readyList
        if((allReady == 1) && (end->state != newArrival) && (tmp == 0)){ 

          //Checks if there is a proces running
          if(bcurRun == 1){
            curRun->processorTime++;
            stepAction = continueRun;
            
            //Issues the ioRequest if the process has one
            if(curRun->ioEvents.size() != 0){  
              if(curRun->ioEvents.begin()->time == curRun->processorTime){
                stepAction = ioRequest;
                ioModule.submitIORequest(time, curRun->ioEvents.front(), *curRun);
                curRun->state = blocked;
                blockedList.push_back(curRun);
                bcurRun = 0;
                curRun->ioEvents.pop_front(); 
              }
            }

            //Checks if the process is done
            else if(curRun->processorTime == curRun->reqProcessorTime){
              stepAction = complete;
              curRun->state = done; 
              bcurRun = 0;
            }
          }

          //If there is not a process currently running
          else{

            //Finds the process in the blocked list that corresponds to the interrupt
            if(interrupts.empty() == 0){
              stepAction = handleInterrupt;
              
              for(unsigned long q=0; q<blockedList.size(); q++){
                if(interrupts.begin()->procID == blockedList[q]->id){ 
                  blockedList[q]->state = ready;
                  readyList.push_back(blockedList[q]);
                  interrupts.pop_front();
                  break;
                }
              }
            }

            //Selects the first process from the readyList to run if there is not one already running
            else if(readyList.size() != 0){
              stepAction = beginRun;
              curRun = readyList.front();
              curRun->state = processing;
              bcurRun = 1;
              readyList.pop_front();
            }
          }
        }
          
        //Checks if all the processes in the processList are done
        allDone = 1;
        list<Process>::iterator run = processList.begin();
        for(unsigned long i=0; i<processList.size(); i++){
          if(run->state != done){
            allDone = 0;
          }
          run++;
        }

        cout << setw(5) << time << "\t"; 
        
        switch(stepAction)
        {
            case admitNewProc:
              cout << "[  admit]\t";
              break;
            case handleInterrupt:
              cout << "[ inrtpt]\t";
              break;
            case beginRun:
              cout << "[  begin]\t";
              break;
            case continueRun:
              cout << "[contRun]\t";
              break;
            case ioRequest:
              cout << "[  ioReq]\t";
              break;
            case complete:
              cout << "[ finish]\t";
              break;
            case noAct:
              cout << "[*noAct*]\t";
              break;
        }

        printProcessStates(processList);

        this_thread::sleep_for(chrono::milliseconds(sleepDuration));
    }

    return 0;
}