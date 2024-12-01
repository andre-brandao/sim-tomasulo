#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <iomanip>

// Constantes de configuração
const int ADD_UNIT_CLOCK = 4;
const int MUL_UNIT_CLOCK = 4;
const int SW_UNIT_CLOCK = 2;
const int ADD_UNIT_QNT = 2;
const int MUL_UNIT_QNT = 2;
const int SW_UNIT_QNT = 2;
const int REGISTER_QNT = 16;

// Definindo a estrutura de uma instrução
class OperationInstruction
{
public:
  std::string op;   // Operação (add, sub, mul, div, etc.)
  std::string dest; // Registrador de destino
  std::string src1; // Operando 1
  std::string src2; // Operando 2
  int execCycles;   // Ciclos de execução restantes
  bool isIssued;
  bool isExecuting; // Indica se está em execução
  bool isCompleted; // Indica se está concluída

  OperationInstruction(std::string op, std::string dest, std::string src1, std::string src2)
      : op(op), dest(dest), src1(src1), src2(src2), execCycles(0), isIssued(false), isExecuting(false), isCompleted(false) {}
};

// Definindo a estrutura de uma unidade funcional
class FunctionalUnit
{
public:
  std::string type;                  // Tipo da unidade funcional (ex: add, mul)
  int latency;                       // Ciclos de latência da unidade funcional
  bool busy;                         // Indica se está ocupada
  OperationInstruction *instruction; // Instrução atualmente em execução

  FunctionalUnit(std::string type, int latency)
      : type(type), latency(latency), busy(false), instruction(nullptr) {}
};

// Definindo a estrutura de um registrador
class Register
{
public:
  std::string name;                  // Nome do registrador
  int value;                         // Valor atual do registrador
  bool busyRead;                     // Indica se o registrador está sendo lido
  bool busyWrite;                    // Indica se o registrador está sendo escrito
  OperationInstruction *instruction; // Instrução que está utilizando o registrador

  Register(std::string name, int value)
      : name(name), value(value), busyRead(false), busyWrite(false), instruction(nullptr) {}
};

// Implementação do algoritmo de Tomasulo
class Scheduler
{
public:
  std::vector<OperationInstruction *> instructions; // Lista de instruções
  std::vector<FunctionalUnit *> addUnits;           // Unidades funcionais de soma
  std::vector<FunctionalUnit *> mulUnits;           // Unidades funcionais de multiplicação
  std::vector<FunctionalUnit *> swUnits;            // Unidades funcionais de store word
  std::vector<Register *> registers;                // Registradores
  std::map<std::string, std::string> rename;
  int cycle; // Ciclo atual
  std::vector<int> cacheMem;
  std::ofstream &outputFile; // Referência ao arquivo de saída

  Scheduler(std::vector<OperationInstruction *> instructions, std::ofstream &outputFile)
      : instructions(instructions), outputFile(outputFile), cycle(1), cacheMem(32, 2)
  {
    addUnits = createFunctionalUnits(ADD_UNIT_QNT, "add", ADD_UNIT_CLOCK);
    mulUnits = createFunctionalUnits(MUL_UNIT_QNT, "mul", MUL_UNIT_CLOCK);
    swUnits = createFunctionalUnits(SW_UNIT_QNT, "sw", SW_UNIT_CLOCK);
    registers = createRegisters(REGISTER_QNT);
  }

  // Destrutor para liberar a memória alocada para as instruções
  ~Scheduler()
  {
    for (const auto &instruction : instructions)
    {
      delete instruction;
    }
    for (const auto &unit : addUnits)
    {
      delete unit;
    }
    for (const auto &unit : mulUnits)
    {
      delete unit;
    }
    for (const auto &unit : swUnits)
    {
      delete unit;
    }
    for (const auto &reg : registers)
    {
      delete reg;
    }
  }

  // Cria as unidades funcionais
  std::vector<FunctionalUnit *> createFunctionalUnits(int numUnits, std::string type, int latency)
  {
    std::vector<FunctionalUnit *> units;
    for (int i = 0; i < numUnits; i++)
    {
      units.push_back(new FunctionalUnit(type, latency));
    }
    return units;
  }

  // Cria os registradores
  std::vector<Register *> createRegisters(int quant)
  {
    std::vector<Register *> reg;
    for (int i = 0; i < quant; i++)
    {
      reg.push_back(new Register("F" + std::to_string(i), 1));
    }
    for (int i = 0; i < quant; i++)
    {
      reg.push_back(new Register("R" + std::to_string(i), 1));
    }
    return reg;
  }

  // Executa o algoritmo de Tomasulo
  void run()
  {
    while (!isExecutionComplete())
    {
      issue();
      execute();
      write();
      showState();
      cycle++;
    }
    outputFile << "\nExecução completa\n";
  }

  void showState()
  {
    // Start of clock cycle state log
    outputFile << "\n------------\nCiclo " << cycle;

    // Log issued instructions
    outputFile << "\n> Lidas: ";
    for (size_t i = 0; i < instructions.size(); i++)
    {
      if (instructions[i]->isIssued)
      {
        outputFile << "\n " << instructions[i]->op << " " << instructions[i]->dest
                   << " " << instructions[i]->src1 << " " << instructions[i]->src2;
      }
    }

    // Log executing instructions
    outputFile << "\n> Em execução:";
    for (size_t i = 0; i < instructions.size(); i++)
    {
      if (instructions[i]->isExecuting)
      {
        outputFile << "\n " << instructions[i]->op << " " << instructions[i]->dest
                   << " " << instructions[i]->src1 << " " << instructions[i]->src2;
      }
    }

    // Log completed instructions
    outputFile << "\n> Concluídas:";
    for (size_t i = 0; i < instructions.size(); i++)
    {
      if (instructions[i]->isCompleted)
      {
        outputFile << "\n " << instructions[i]->op << " " << instructions[i]->dest
                   << " " << instructions[i]->src1 << " " << instructions[i]->src2;
      }
    }

    // detectDependencies(outputFile);

    // Integrated register status logging
    outputFile << "\n\n> Status dos Registradores:";
    outputFile << "\n"
               << std::left
               << std::setw(12) << "Registrador"
               << std::setw(10) << "Valor"
               << std::setw(15) << "(Leitura)"
               << std::setw(15) << "(Escrita)"
               << "Instrução Associada\n";
    outputFile << std::string(70, '-') << "\n";

    // Iterate through registers and log their states
    for (const auto &reg : registers)
    {
      // Determine read and write status
      std::string readStatus = reg->busyRead ? "Ocupado" : "Livre";
      std::string writeStatus = reg->busyWrite ? "Ocupado" : "Livre";

      // Prepare instruction information
      std::string instrInfo = "Nenhuma";
      if (reg->instruction)
      {
        instrInfo = reg->instruction->op + " " +
                    reg->instruction->dest + " " +
                    reg->instruction->src1 + " " +
                    reg->instruction->src2;
      }

      // Write register state to file
      outputFile << std::left
                 << std::setw(12) << reg->name
                 << std::setw(10) << reg->value
                 << std::setw(15) << readStatus
                 << std::setw(15) << writeStatus
                 << instrInfo << "\n";
    }

    // Additional separator for readability
    outputFile << std::string(70, '-') << "\n\n";
  }

  // Verifica se a execução está completa
  bool isExecutionComplete()
  {
    for (size_t i = 0; i < instructions.size(); i++)
    {
      if (!instructions[i]->isCompleted)
      {
        return false;
      }
    }
    return true;
  }

  void renameRegister(Register *target, size_t index)
  {
    // Declara o início dos registradores temporários
    size_t posTempReg = registers.size() / 2;
    std::string newName = "R0";

    // Loop para encontrar registrador temporário disponível
    while (registers[posTempReg]->busyRead || registers[posTempReg]->busyWrite)
    {
      posTempReg++;
      if (posTempReg >= registers.size())
      {
        return;
      }
      else
      {
        newName = registers[posTempReg]->name;
      }
    }

    if (rename.find(target->name) != rename.end())
    {
      rename[newName] = rename[target->name];
    }
    else
    {
      rename[newName] = target->name;
    }

    // Renomear próximas ocorrências do registrador alvo
    for (size_t i = index; i < instructions.size(); i++)
    {
      if (instructions[i]->dest == target->name)
      {
        instructions[i]->dest = newName;
      }

      if (instructions[i]->src1 == target->name)
      {
        instructions[i]->src1 = newName;
      }

      if (instructions[i]->src2 == target->name)
      {
        instructions[i]->src2 = newName;
      }
    }
  }

  // Faz o estágio de emissão (issue)
  void issue()
  {
    size_t constrain = instructions.size();
    if (cycle < instructions.size())
    {
      constrain = cycle;
    }
    for (size_t i = 0; i < constrain; i++)
    {
      OperationInstruction *instruction = instructions[i];
      FunctionalUnit *unit = nullptr;
      if (instruction->isExecuting || instruction->isCompleted || !instruction->isIssued)
      {
        instruction->isIssued = true;
        unit = nullptr;
      }
      else if (instruction->op == "add" || instruction->op == "sub")
      {
        unit = findAvailableUnit(addUnits);
      }
      else if (instruction->op == "mul" || instruction->op == "div")
      {
        unit = findAvailableUnit(mulUnits);
      }
      else if (instruction->op == "sw" || instruction->op == "lw")
      {
        unit = findAvailableUnit(swUnits);
      }

      if (unit)
      {
        // Ajusta para caso de SW e LW
        Register *aux;
        if (instruction->op == "sw" || instruction->op == "lw")
        {
          aux = new Register("aux", std::stoi(instruction->src1));
        }
        else
        {
          aux = getRegister(instruction->src1);
        }

        // Verifica se os operandos estão disponíveis
        Register *destRegister = getRegister(instruction->dest);
        Register *src1Register = aux;
        Register *src2Register = getRegister(instruction->src2);

        // Dependência falsa
        if (destRegister->busyRead || destRegister->busyWrite)
        {
          renameRegister(destRegister, i);
        }

        // Dependência verdadeira
        if (!src1Register->busyWrite && !src2Register->busyWrite)
        {
          // Define a instrução como executando
          instruction->isExecuting = true;
          instruction->execCycles = unit->latency;

          // Define a unidade funcional como ocupada
          unit->busy = true;
          unit->instruction = instruction;

          // Atualiza os registradores utilizados pela instrução
          destRegister->busyWrite = true;
          destRegister->instruction = instruction;

          src1Register->busyRead = true;
          src1Register->instruction = instruction;

          src2Register->busyRead = true;
          src2Register->instruction = instruction;
        }
      }
    }
  }

  // Faz o estágio de execução
  void execute()
  {
    for (size_t i = 0; i < addUnits.size(); i++)
    {
      FunctionalUnit *unit = addUnits[i];
      if (unit->busy)
      {
        unit->instruction->execCycles--;
        if (unit->instruction->execCycles == 0)
        {
          unit->instruction->isExecuting = false;
        }
      }
    }

    for (size_t i = 0; i < mulUnits.size(); i++)
    {
      FunctionalUnit *unit = mulUnits[i];
      if (unit->busy)
      {
        unit->instruction->execCycles--;
        if (unit->instruction->execCycles == 0)
        {
          unit->instruction->isExecuting = false;
        }
      }
    }

    for (size_t i = 0; i < swUnits.size(); i++)
    {
      FunctionalUnit *unit = swUnits[i];
      if (unit->busy)
      {
        unit->instruction->execCycles--;
        if (unit->instruction->execCycles == 0)
        {
          unit->instruction->isExecuting = false;
        }
      }
    }
  }

  // Faz o estágio de escrita
  void write()
  {
    writeExecution(addUnits);
    writeExecution(mulUnits);
    writeExecution(swUnits);
  }

  // Executa as funções da escrita
  void writeExecution(std::vector<FunctionalUnit *> &unitType)
  {
    for (size_t i = 0; i < unitType.size(); i++)
    {
      FunctionalUnit *unit = unitType[i];
      if (unit->busy && !unit->instruction->isExecuting && !unit->instruction->isCompleted)
      {
        unit->instruction->isCompleted = true;
        unit->busy = false;

        // Atualiza o valor do registrador de destino
        Register *destRegister = getRegister(unit->instruction->dest);
        Register *src1Register = getRegister(unit->instruction->src1);
        Register *src2Register = getRegister(unit->instruction->src2);

        if (unit->instruction->op == "sw" || unit->instruction->op == "lw")
        {
          int offset = performOperation("add", std::stoi(unit->instruction->src1), src2Register->value);

          if (unit->instruction->op == "sw")
          {
            cacheMem[offset % cacheMem.size()] = destRegister->value;
          }
          else if (unit->instruction->op == "lw")
          {
            destRegister->value = cacheMem[offset % cacheMem.size()];
          }
        }
        else
        {
          destRegister->value = performOperation(unit->instruction->op, src1Register->value, src2Register->value);

          src1Register->busyRead = false;
          src1Register->instruction = nullptr;
        }

        // Libera os registradores utilizados pela instrução
        destRegister->busyWrite = false;
        destRegister->instruction = nullptr;

        if (rename.find(destRegister->name) != rename.end())
        {
          returnRenamed(destRegister, i);
        }

        src2Register->busyRead = false;
        src2Register->instruction = nullptr;
      }
    }
  }

  void returnRenamed(Register *target, size_t index)
  {
    // Renomear os valores na lista de instrução
    for (size_t i = 0; i < instructions.size(); i++)
    {
      if (instructions[i]->dest == target->name)
      {
        instructions[i]->dest = rename[target->name];
      }

      if (instructions[i]->src1 == target->name)
      {
        instructions[i]->src1 = rename[target->name];
      }

      if (instructions[i]->src2 == target->name)
      {
        instructions[i]->src2 = rename[target->name];
      }
    }

    // Remove da lista de renomeados
    rename.erase(target->name);
  }

  // Encontra uma unidade funcional disponível
  FunctionalUnit *findAvailableUnit(std::vector<FunctionalUnit *> &units)
  {
    for (size_t i = 0; i < units.size(); i++)
    {
      if (!units[i]->busy)
      {
        return units[i];
      }
    }
    return nullptr;
  }

  // Obtém o registrador pelo nome
  Register *getRegister(const std::string &name)
  {
    for (size_t i = 0; i < registers.size(); i++)
    {
      if (registers[i]->name == name)
      {
        return registers[i];
      }
    }
    return nullptr;
  }

  // Executa a operação matemática
  int performOperation(const std::string &op, int src1, int src2)
  {
    if (op == "add")
    {
      return src1 + src2;
    }
    else if (op == "sub")
    {
      return src1 - src2;
    }
    else if (op == "mul")
    {
      return src1 * src2;
    }
    else if (op == "div")
    {
      return src1 / src2;
    }
    return 0;
  }
  // void detectDependencies(std::ofstream &outputFile)
  // {
  //     outputFile << "\n> Dependências de Instruções Recém-Lidas:";

  //     for (size_t i = 0; i < instructions.size(); i++)
  //     {
  //         // Only check recently issued instructions
  //         if (instructions[i]->isIssued && !instructions[i]->isExecuting && !instructions[i]->isCompleted)
  //         {
  //             // Check source registers for dependencies
  //             std::vector<std::string> dependencyTypes = checkRegisterDependencies(instructions[i]);

  //             if (!dependencyTypes.empty())
  //             {
  //                 outputFile << "\n Instrução: "
  //                            << instructions[i]->op << " "
  //                            << instructions[i]->dest << " "
  //                            << instructions[i]->src1 << " "
  //                            << instructions[i]->src2;

  //                 for (const auto &depType : dependencyTypes)
  //                 {
  //                     outputFile << "\n  - " << depType;
  //                 }
  //             }
  //         }
  //     }
  // }
  std::vector<std::string> checkRegisterDependencies(OperationInstruction *instruction)
  {
    std::vector<std::string> dependencies;

    // Check destination register dependencies
    Register *destReg = getRegister(instruction->dest);
    if (destReg->busyRead || destReg->busyWrite)
    {
      // Only add dependency if the source instruction has not executed
      if (destReg->instruction &&
          !(destReg->instruction->isCompleted))
      {
        dependencies.push_back("Dependência Falsa (WAR/WAW) no registrador de destino: " + destReg->name);
      }
    }

    // Check source register 1 dependencies
    Register *src1Reg = getRegister(instruction->src1);
    if (src1Reg && (src1Reg->busyRead || src1Reg->busyWrite))
    {
      // Only add dependency if the source instruction has not executed
      if (src1Reg->instruction &&
          !(src1Reg->instruction->isCompleted))
      {
        dependencies.push_back("Dependência Verdadeira (RAW) no primeiro registrador fonte: " + src1Reg->name);
      }
    }

    // Check source register 2 dependencies
    Register *src2Reg = getRegister(instruction->src2);
    if (src2Reg && (src2Reg->busyRead || src2Reg->busyWrite))
    {
      // Only add dependency if the source instruction has not executed
      if (src2Reg->instruction &&
          !(src2Reg->instruction->isCompleted))
      {
        dependencies.push_back("Dependência Verdadeira (RAW) no segundo registrador fonte: " + src2Reg->name);
      }
    }

    return dependencies;
  }
};

// Exemplo de uso
int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    std::cerr << "Uso: " << argv[0] << " <arquivo_de_instrucoes> <arquivo_de_saida>" << std::endl;
    return 1;
  }

  // Leitura das instruções a partir de um arquivo de texto
  std::vector<OperationInstruction *> instructions;
  std::ifstream input(argv[1]);

  if (!input.is_open())
  {
    std::cerr << "Erro ao abrir o arquivo de instruções." << std::endl;
    return 1;
  }

  std::ofstream outputFile(argv[2]);
  if (!outputFile.is_open())
  {
    std::cerr << "Erro ao abrir o arquivo de saída." << std::endl;
    return 1;
  }

  std::string line;
  while (std::getline(input, line))
  {
    std::istringstream iss(line);
    std::string op, dest, src1, src2;
    iss >> op >> dest >> src1 >> src2;
    instructions.push_back(new OperationInstruction(op, dest, src1, src2));
  }

  input.close();

  Scheduler tomasulo(instructions, outputFile);
  tomasulo.run();

  // Fechar o arquivo de saída
  outputFile.close();

  return 0;
}