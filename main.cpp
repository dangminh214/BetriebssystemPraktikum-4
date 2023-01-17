#include <cstddef>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include <cassert>

static unsigned int pid = 0;
const unsigned int waitTime = 4;
const unsigned int PAGE_SIZE = 64;
const unsigned int FRAME_SIZE = 4;
unsigned int runningSimulation = 1;
unsigned int quantumCounter = 0;
unsigned int quantum = 5;
const unsigned int MAX_ADDR = 1023;
enum class state {
    running = 0, ready = 1, blocked = 2
};

struct Seite {
    bool present = 0;
    bool modified = 0;
    bool accessed = 0;
    unsigned int index = 9999;
    unsigned short int accessTime = 0;
    unsigned int seitenfehler = 0;
};

struct Process {
    unsigned int processId;
    std::string name;
    std::vector<std::string> befehle;
    unsigned int index;
    unsigned int data;
    state processStatus;
    unsigned int wait;
    std::vector<Seite> Seitentabelle;

    Process() {
        processId = pid;
        pid++;
        data = 0;
        index = 0;
        processStatus = state::ready;
        wait = 0;
        Seitentabelle.resize(16);
    }
};

struct Scheduler {
    std::vector<Process> readyProcesses;
    std::vector<Process> waitingProcesses;
    std::vector<Process> processes;
    std::vector<std::vector<unsigned char>> disk;
    unsigned char RAM[FRAME_SIZE * PAGE_SIZE] = { NULL };

    void printAll() {
        std::cout << "Ram:" << std::endl;
        for(unsigned int i = 0; i < FRAME_SIZE * PAGE_SIZE; i++) {
            std::cout << i << ": "  << int(RAM[i]) << std::endl;
        }
        std::cout << "\n\n\n";
        std::cout << "Disk:" << std::endl;
        for (unsigned int i = 0; i < disk.size(); i++) {
            std::cout << "ProcessID " << i << std::endl;
            for (unsigned int j = 0; j < MAX_ADDR+1; j++) {
                std::cout << j << ": " << int(disk[i][j]) << std::endl;
            }
        }

        std::cout << std::endl;
    }

    void updateDisk() {
        for (auto& p : processes) {
            for (unsigned int i = 0; i < p.Seitentabelle.size(); i++) {
                if (p.Seitentabelle.at(i).index < FRAME_SIZE) {
                        for (unsigned int j = 0; j < PAGE_SIZE; j++) {
                            disk.at(p.processId).at(i * PAGE_SIZE + j) = RAM[p.Seitentabelle.at(i).index * PAGE_SIZE + j];
                        }
                }
            }
        }
    }

    void updateAccessTime() {
        for (auto& p : processes) {
            for (auto& s : p.Seitentabelle) {
                if (s.accessed == true) {
                    if (s.index < FRAME_SIZE) {
                        s.accessTime++;
                        if (s.accessTime == 5) {
                            s.accessed = false;
                            s.accessTime = 0;
                        }
                    }

                }
            }
        }
    }

    //swap function for NRU

    bool swap(bool accessed, bool modified) {
        for (auto& p : processes) {
            for (unsigned int i = 0; i < p.Seitentabelle.size(); i++) {
                if (p.Seitentabelle.at(i).index < FRAME_SIZE) {
                    if (p.Seitentabelle.at(i).accessed == accessed && p.Seitentabelle.at(i).modified == modified) {
                        for (unsigned int j = 0; j < PAGE_SIZE; j++) {
                            disk.at(p.processId).at(i * PAGE_SIZE + j) = RAM[p.Seitentabelle.at(i).index * PAGE_SIZE + j];
                        }
                        p.Seitentabelle.at(i).index = i + FRAME_SIZE;
                        return true;
                    }
                }
            }
        }
        return false;
    }

    //Not Recently Used Algorithmus
    void NRU() {
        if(swap(false, false)) return;
        if(swap(false, true)) return;
        if(swap(true, false)) return;
        if(swap(true, true)) return;
        assert( false );
    }

    unsigned short int findFrame() {

        unsigned short int frameNumber = 0;
        bool check = false;
            while(!check){
            check = false;
            for (auto& p : processes) {
                    for (auto& s : p.Seitentabelle) {
                        if ((s.index == frameNumber) && (s.present)) {
                            frameNumber++;
                            check = false;
                            break;
                        }
                        else check = true;
                    }
                    if (!check) break;
                }

            }
            if(frameNumber < FRAME_SIZE) return frameNumber;
        else return 9999;


    }

    void getPage(unsigned short int frameNumber, unsigned int pID, unsigned short int pageNumber) {
        for (unsigned int i = 0; i < PAGE_SIZE; i++) {
            RAM[frameNumber * PAGE_SIZE + i] = disk.at(pID).at(pageNumber * PAGE_SIZE + i);
        }
    }

    bool getReadyProcess() {
        if (readyProcesses.size() > 0) return true;
        return false;
    }

    void updateWait() {
        if (waitingProcesses.size() > 0) {
            for (unsigned int i = 0; i < waitingProcesses.size(); i++) {
                waitingProcesses.at(i).wait--;
                if (waitingProcesses.at(i).wait == 0) {
                    waitingProcesses.at(i).processStatus = state::ready;
                    if (!getReadyProcess()) waitingProcesses.at(i).processStatus = state::running;
                    readyProcesses.push_back(waitingProcesses.at(i));
                    waitingProcesses.erase(waitingProcesses.begin() + i);
                }
            }
        }
        else {
            return;
        }
    }

    void blockProcess(Process process) {
        if (process.processStatus == state::blocked) return;
        process.processStatus = state::blocked;
        process.wait = waitTime;
        waitingProcesses.push_back(process);
        for (unsigned int i = 0; i < readyProcesses.size(); i++) {
            if (process.processId == readyProcesses.at(i).processId)
                readyProcesses.erase(readyProcesses.begin() + i);
        }
    }

    Process& getRunningProcess() {
        for (Process& p : readyProcesses) {
            if (p.processStatus == state::running) return p;
        }

    }

    void switchProcess() {
        readyProcesses.at(0).processStatus = state::running;
    }

    void destroyProcess() {
        unsigned int id = 0;
        for (unsigned int i = 0; i < readyProcesses.size(); i++) {
            if (readyProcesses.at(i).processStatus == state::running) {
                id = readyProcesses.at(i).processId;
                readyProcesses.erase(readyProcesses.begin() + i);
            }
        }
        for (unsigned int i = 0; i < processes.size(); i++) {

            if (processes.at(i).processId == id) {
                calcSeitenFehler(id);
                processes.erase(processes.begin() + i);
            }
        }

        if (getReadyProcess()) switchProcess();
        else if (waitingProcesses.size() == 0) runningSimulation = 0;
    }

    void calcSeitenFehler(unsigned int id) {
        unsigned int sum = 0;
        for (unsigned int i = 0; i < processes.at(id).Seitentabelle.size(); i++) {
            sum += processes.at(id).Seitentabelle.at(i).seitenfehler;
        }

        std::cout << std::endl;

        std::cout << "Process " << id << ":" << sum << " Seitenfehler" << std::endl;
        std::cout << std::endl;

    }

};


struct MMU {

    unsigned short int resolve(Scheduler& betriebssystem, std::string sAdress, Process& process, bool modified = 0) {
        unsigned int short virtualAdress = std::stoi(sAdress, 0, 16); //to base 16

        if (virtualAdress > MAX_ADDR) {
            std::cout << "Trying to read/write from Memory where not allowed" << std::endl;
            return NULL;
        }

        unsigned short int pageNumber = virtualAdress / PAGE_SIZE; //seite
        unsigned short int offset = virtualAdress % PAGE_SIZE; //offset
        // Einfacher wäre
        // if NOT present
        //    if Memory Full
        //       NRU  -- nun ist Memory NOT full
        //    Put Page in empty page frame
        // Nun ist Present Treue
        // set accessed , accessTime, modified, etc...
        if (betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].present == 1) { //Seite ist schon in RAM oder Disk
            if (betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].index <= 3) { //Liegt die Seite in einem Seitenrahmen
                betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].accessed = 1;
                betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].accessTime = 0;
                betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].modified = modified;
                return betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].index * PAGE_SIZE + offset; //return Position im RAM
            }
//            else {
//                //Seite liegt auf der Disk und muss von dort geholt werden
//                //Process index gibt an welche Stelle der Disk und Page gibt an welche Seite
//                betriebssystem.NRU();
//                unsigned short int frameNumber = betriebssystem.findFrame();
//                betriebssystem.getPage(frameNumber, process.processId, pageNumber);
//                betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].index = frameNumber;
//                betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].accessed = 1;
//                betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].accessTime = 0;
//                betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].seitenfehler++;
//                betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].modified = modified;
//                return betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].index * PAGE_SIZE + offset;
//            }
        }
        else {
            //Seite noch nicht existiert
            //Look at Seitenrahmen in all Seitentabelle
                unsigned short int frameNumber = betriebssystem.findFrame();
                if (frameNumber < 4) {//ersten freien frame reservieren
                    betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].present = 1;
                    betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].index = frameNumber;
                    betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].accessed = 1;
                    betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].accessTime = 0;
                    betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].seitenfehler++;
                    betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].modified = modified;
                    return betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].index * PAGE_SIZE + offset;
                }
                //swap when no frame frei
                else {
                    betriebssystem.NRU();
                    frameNumber = betriebssystem.findFrame();
                    betriebssystem.getPage(frameNumber, process.processId, pageNumber);
                    betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].present = 1;
                    betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].index = frameNumber;
                    betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].accessed = 1;
                    betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].accessTime = 0;
                    betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].seitenfehler++;
                    betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].modified = modified;
                    return betriebssystem.processes.at(process.processId).Seitentabelle[pageNumber].index * PAGE_SIZE + offset;
                }
        }
    }
};


struct Cpu {
    unsigned int takt = 0;
    unsigned int pc = 0;
    unsigned int ACC = 0;

    void write(Scheduler& betriebssystem, unsigned short int adress) {
        betriebssystem.RAM[adress] = (char)ACC;
    }

    void read(Scheduler& betriebssystem, unsigned short int adress) {
        ACC = betriebssystem.RAM[adress];
    }

    void init(Scheduler& scheduler, MMU& mmu) {
        createProcess(scheduler, "init");
        execute(scheduler, scheduler.readyProcesses.front(), mmu);
    }

    void createProcess(Scheduler& scheduler, std::string program) {
        std::string path =  program;
        std::string opcode, variable;
        std::ifstream infile(path);
        Process newProcess;
        newProcess.name = program;
        newProcess.index = 0;
        newProcess.processStatus = state::running;

        while (infile >> opcode) {
            if (opcode != "P" && opcode != "Z") {
                infile >> variable; newProcess.befehle.push_back(opcode + " " + variable);
            }
            else {
                variable = " ";
                newProcess.befehle.push_back(opcode + variable);
            }

        }
        scheduler.readyProcesses.push_back(newProcess);
        scheduler.processes.push_back(newProcess);
        scheduler.disk.push_back(std::vector<unsigned char>(1024));
    }

    void execute(Scheduler& scheduler, Process process, MMU& mmu) {
        while (runningSimulation == 1) {
            scheduler.updateWait();
            scheduler.updateAccessTime();
            if (scheduler.getReadyProcess()) {
                quantumCounter = 0;
                process = scheduler.getRunningProcess();
                pc = process.index;
                ACC = process.data;
                takt++;
            }
            else {
                takt++;
                std::cout << std::setw(10) << takt << std::setw(10) << process.processId << std::setw(10) << process.name << std::setw(10) << pc << "/" << process.befehle.size() - 1 << std::setw(10) << ACC << std::setw(13) << "--" << std::endl;
                continue;
            }

            while (pc < process.befehle.size()) {
                quantumCounter++;
                std::string command = process.befehle.at(pc);
                std::cout << std::setw(10) << takt << std::setw(10) << process.processId << std::setw(10) << process.name << std::setw(10) << pc << "/" << process.befehle.size() - 1 << std::setw(10) << ACC << std::setw(13) << command << std::endl;
                std::string opcode, variable;
                std::string delimiter = " ";
                size_t pos = command.find(delimiter);
                opcode = command.substr(0, command.find(delimiter));
                variable = command.substr(pos + 1);
                if (quantumCounter % quantum == 0) {
                    opcode = "P";

                    pc -= 1;
                }
                if (opcode.at(0) == 'L') {
                    ACC = std::stoi(variable);
                }
                else if (opcode.at(0) == 'A') {
                    ACC += std::stoi(variable);
                }
                else if (opcode.at(0) == 'S') {
                    ACC -= std::stoi(variable);
                }
                else if (opcode.at(0) == 'P') {
                    process.index = pc + 1;
                    process.data = ACC;
                    scheduler.blockProcess(process);
                    if (scheduler.getReadyProcess()) {
                        scheduler.switchProcess();
                        break;
                    }
                    else break;

                }
                else if (opcode.at(0) == 'X') {
                    scheduler.getRunningProcess().index = pc + 1;
                    scheduler.getRunningProcess().data = ACC;
                    scheduler.getRunningProcess().processStatus = state::ready;
                    createProcess(scheduler, variable);
                    break;
                }
                else if (opcode.at(0) == 'Z') {
                    scheduler.destroyProcess();
                    break;
                }
                else if (opcode.at(0) == 'R') {
                    unsigned short int ramAdress = mmu.resolve(scheduler, variable, process);
                    read(scheduler, ramAdress);
                }
                else if (opcode.at(0) == 'W') {
                    unsigned short int ramAdress = mmu.resolve(scheduler, variable, process, 1);
                    write(scheduler, ramAdress);

                }
                pc++;
                takt++;
                scheduler.updateWait();
                scheduler.updateAccessTime();
                scheduler.updateDisk();
            }
        }
    }
};


int main() {
    std::cout << std::setw(10) << "Takt " << std::setw(10) << "PID " << std::setw(10) << "Process " << std::setw(10) << "PC " << std::setw(10) << "Acc " << std::setw(13) << "Befehl" << std::endl;
    Cpu cpu;
    Scheduler scheduler;;
    MMU mmu;
    cpu.init(scheduler, mmu);
    std::cout << " Simulation end" << std::endl;
    scheduler.printAll();
    return 0;
}
