#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

// Classe que representa uma operação
class Operation {
public:
  std::string opcode;   // Tipo de operação (ex: soma, subtração)
  std::string target;   // Registrador de destino
  std::string operand1; // Primeiro operando
  std::string operand2; // Segundo operando
  int cyclesLeft;       // Ciclos restantes para execução
  bool issued;
  bool executing;
  bool completed;

  Operation(std::string opcode, std::string target, std::string operand1,
            std::string operand2)
      : opcode(opcode), target(target), operand1(operand1), operand2(operand2),
        cyclesLeft(0), issued(false), executing(false), completed(false) {}
};

// Classe que modela uma unidade computacional
class ComputeUnit {
public:
  std::string category;
  int delay;
  bool inUse;
  Operation *currentOp;

  ComputeUnit(std::string category, int delay)
      : category(category), delay(delay), inUse(false), currentOp(nullptr) {}
};

// Classe que define um registrador
class CPURegister {
public:
  std::string label;
  int data;
  bool readBusy;
  bool writeBusy;
  Operation *usingOp;

  CPURegister(std::string label, int data)
      : label(label), data(data), readBusy(false), writeBusy(false),
        usingOp(nullptr) {}
};

// Classe principal do algoritmo
class Scheduler {
public:
  std::vector<Operation *> opQueue;
  std::vector<ComputeUnit *> arithmeticUnits;
  std::vector<ComputeUnit *> memoryUnits;
  std::vector<CPURegister *> cpuRegisters;
  std::map<std::string, std::string> regRenames;
  int currentCycle;

  Scheduler(std::vector<Operation *> opQueue, std::map<std::string, int> config)
      : opQueue(opQueue), currentCycle(0) {
    arithmeticUnits =
        initializeUnits(config["arithUnits"], "arith", config["arithLatency"]);
    memoryUnits =
        initializeUnits(config["memUnits"], "mem", config["memLatency"]);
    cpuRegisters = initializeRegisters(config["registerCount"]);
  }

  std::vector<ComputeUnit *> initializeUnits(int count, std::string type,
                                             int delay) {
    std::vector<ComputeUnit *> units;
    for (int i = 0; i < count; ++i) {
      units.push_back(new ComputeUnit(type, delay));
    }
    return units;
  }

  std::vector<CPURegister *> initializeRegisters(int count) {
    std::vector<CPURegister *> regs;
    for (int i = 0; i < count; ++i) {
      regs.push_back(new CPURegister("R" + std::to_string(i), 0));
    }
    return regs;
  }

  void executeCycle() {
    issueOperations();
    processExecution();
    completeWriteBack();
    printCycleStatus();
    currentCycle++;
  }

  void issueOperations() {
    for (Operation *op : opQueue) {
      if (!op->issued) {
        ComputeUnit *availableUnit = findAvailableUnit(op);
        if (availableUnit) {
          op->issued = true;
          op->executing = true;
          op->cyclesLeft = availableUnit->delay;
          availableUnit->inUse = true;
          availableUnit->currentOp = op;
        }
      }
    }
  }

  ComputeUnit *findAvailableUnit(Operation *op) {
    std::vector<ComputeUnit *> *unitList = &arithmeticUnits;
    if (op->opcode == "load" || op->opcode == "store") {
      unitList = &memoryUnits;
    }
    for (ComputeUnit *unit : *unitList) {
      if (!unit->inUse) {
        return unit;
      }
    }
    return nullptr;
  }

  void processExecution() {
    for (ComputeUnit *unit : arithmeticUnits) {
      if (unit->inUse && unit->currentOp->executing) {
        unit->currentOp->cyclesLeft--;
        if (unit->currentOp->cyclesLeft == 0) {
          unit->currentOp->executing = false;
        }
      }
    }

    for (ComputeUnit *unit : memoryUnits) {
      if (unit->inUse && unit->currentOp->executing) {
        unit->currentOp->cyclesLeft--;
        if (unit->currentOp->cyclesLeft == 0) {
          unit->currentOp->executing = false;
        }
      }
    }
  }

  void completeWriteBack() {
    for (ComputeUnit *unit : arithmeticUnits) {
      if (unit->inUse && !unit->currentOp->executing) {
        unit->currentOp->completed = true;
        unit->inUse = false;
      }
    }

    for (ComputeUnit *unit : memoryUnits) {
      if (unit->inUse && !unit->currentOp->executing) {
        unit->currentOp->completed = true;
        unit->inUse = false;
      }
    }
  }

  bool isAllCompleted() {
    for (Operation *op : opQueue) {
      if (!op->completed) {
        return false;
      }
    }
    return true;
  }

  void printCycleStatus() {
    std::cout << "Cycle " << currentCycle << ":\n";

    // Print Operation States
    std::cout << "Operations Status:\n";
    for (Operation *op : opQueue) {
      std::cout << "  " << op->opcode << " " << op->target
                << " (issued: " << op->issued
                << ", executing: " << op->executing
                << ", completed: " << op->completed << ")\n";
    }

    // Print Compute Unit States
    std::cout << "Compute Units Status:\n";
    for (ComputeUnit *unit : arithmeticUnits) {
      std::cout << "  " << unit->category << " unit, in use: " << unit->inUse;
      if (unit->inUse) {
        std::cout << ", operation: " << unit->currentOp->opcode;
      }
      std::cout << "\n";
    }

    // Print Register States
    std::cout << "Registers Status:\n";
    for (CPURegister *reg : cpuRegisters) {
      std::cout << "  " << reg->label << " (data: " << reg->data
                << ", readBusy: " << reg->readBusy
                << ", writeBusy: " << reg->writeBusy << ")\n";
    }

    std::cout << "\n";
  }

  void simulate() {
    while (!isAllCompleted()) {
      executeCycle();
    }
    std::cout << "Execution completed in " << currentCycle << " cycles.\n";
  }

  ~Scheduler() {
    // Clean up dynamically allocated memory
    for (Operation *op : opQueue) {
      delete op;
    }
    for (ComputeUnit *unit : arithmeticUnits) {
      delete unit;
    }
    for (ComputeUnit *unit : memoryUnits) {
      delete unit;
    }
    for (CPURegister *reg : cpuRegisters) {
      delete reg;
    }
  }
};

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <instruction_file>" << std::endl;
    return 1;
  }

  std::vector<Operation *> operations;
  std::ifstream input(argv[1]);
  if (!input.is_open()) {
    std::cerr << "Error opening file: " << argv[1] << std::endl;
    return 1;
  }

  std::string line;
  while (std::getline(input, line)) {
    std::istringstream iss(line);
    std::string op, target, src1, src2;
    iss >> op >> target >> src1 >> src2;
    operations.push_back(new Operation(op, target, src1, src2));
  }
  input.close();

  std::map<std::string, int> config = {{"arithUnits", 2},
                                       {"memUnits", 1},
                                       {"arithLatency", 4},
                                       {"memLatency", 2},
                                       {"registerCount", 8}};

  Scheduler scheduler(operations, config);
  scheduler.simulate();

  return 0;
}
