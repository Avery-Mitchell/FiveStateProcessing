#include "process.h"
#include "ioModule.h"
#include "processMgmt.h"

#include <chrono> // for sleep
#include <thread> // for sleep

int main(int argc, char* argv[])
{
    // single thread processor
    // it's either processing something or it's not
    // bool processorAvailable = true;

    // vector of processes, processes will appear here when they are created by
    // the ProcessMgmt object (in other words, automatically at the appropriate time)
    list<Process> processList;
    
    // vector of iterators for that point to ready processes in the processList
    vector<list<Process>::iterator> readyList;

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

    // Do not touch
    long time = 1;
    long sleepDuration = 50;
    string file;
    stringstream ss;
    enum stepActionEnum {noAct, admitNewProc, handleInterrupt, beginRun, continueRun, ioRequest, complete} stepAction;

    // Do not touch
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

    time = 0;
    //processorAvailable = true;

    //TODO: More logic needed to keep the while loop going
    while(processMgmt.moreProcessesComing() || allDone != 1)
    {
        //Update our current time step
        ++time;

        //let new processes in if there are any
        processMgmt.activateProcesses(time);

        //update the status for any active IO requests
        ioModule.ioProcessing(time);


        stepAction = noAct;

        //This loop is responsible for adding processes to my ready list and selecting the process to run (before IO request)
        list<Process>::iterator itr = processList.begin();
        list<Process>::iterator end = processList.begin();
        int tmp = 0;
        for(unsigned long x=0; x<processList.size(); x++){
          if(itr->state == 0){
            readyList.push_back(itr);
            itr->state = ready;
          }
          else if(itr->state == 1){
            bcurRun = 1;
            curRun = itr;
          }
          else if(itr->state == 3 && tmp == 0){
            readyList.push_back(itr);
            itr->state = ready;
            tmp++;
            stepAction = admitNewProc;
          }
          itr++;
          end = itr;
        }
        

        // !!-- This is how to access the state of items in the processList --!!        
        //list<Process>::iterator itr1 = processList.begin();
        //itr1 -> state = ready;
        //cout << itr1 -> state << endl;
        
        //for(const auto& x : readyList){
        //  x->state =?
        //}

        if(processMgmt.moreProcessesComing()){
          stepAction = admitNewProc;
        }

        //stepAction = continueRun;       //runnning process is still running
        //stepAction = ioRequest;         //running process issued an io request
        //stepAction = complete;          //running process is finished
        //stepAction = admitNewProc;      //admit a new process into 'ready'
        //stepAction = handleInterrupt;   //handle an interrupt
        //stepAction = beginRun;          //start running a process


        //IO request
        //ioModule.submitIORequest(time, curRun->ioEvents.front(), *curRun);

        if(!processMgmt.moreProcessesComing() && (end->state != newArrival) && tmp == 0){ 
          if(bcurRun == 1){
            curRun->processorTime++;
            stepAction = continueRun;
            if(curRun->processorTime == curRun->reqProcessorTime){
              curRun->state = done; 
              bcurRun = 0;
              stepAction = complete;
            }
          }
          else{
            if(readyList.size() != 0){
              curRun = readyList[0];
              curRun->state = processing;
              bcurRun = 1;
              stepAction = beginRun;
            }
          }
        }
          
        readyList.clear();

        allDone = 1;
        list<Process>::iterator run = processList.begin();
        for(unsigned long i=0; i<processList.size(); i++){
          if(run->state != done){
            allDone = 0;
          }
          run++;
        }

        // Leave the below alone (at least for final submission, we are counting on the output being in expected format)
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

        // You may wish to use a second vector of processes (you don't need to, but you can)
        printProcessStates(processList); // change processList to another vector of processes if desired

        this_thread::sleep_for(chrono::milliseconds(sleepDuration));
    }

    return 0;
}
