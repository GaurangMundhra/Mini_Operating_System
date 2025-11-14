#include <iostream>
#include <fstream>
#include <string>
#include <iomanip> // Used for setw, setfill

using namespace std;

class OS
{
private:
  // 1. Hardware Definition
  char M[100][4];
  char IR[4];
  char R[4];
  int IC;
  bool C;
  int SI;

  char buffer[40];
  bool job_terminated;

  fstream infile;
  fstream outfile;

  // Internal functions
  void init();
  void ExecuteUserProgram();
  void MOS();

  // --- Visualization Functions ---

  // Updated to handle the |G|D|2|0| format
  string charArrToString(char arr[4], bool separators = false)
  {
    string s = "";
    if (separators)
    {
      for (int i = 0; i < 4; i++)
      {
        s += '|';
        // Replace uninitialized/space chars with a dot for clarity
        s += (arr[i] == ' ' || arr[i] == '\0') ? '.' : arr[i];
      }
      s += '|'; // Add trailing separator
    }
    else
    {
      for (int i = 0; i < 4; i++)
      {
        s += (arr[i] == ' ' || arr[i] == '\0') ? '.' : arr[i];
      }
    }
    return s;
  }

  // *** THIS IS THE UPDATED DASHBOARD FUNCTION ***
  void printDashboard(string message)
  {
    cout << "\n\n=======================================================";
    cout << "\n[ STATUS: " << message << " ]";
    cout << "\n-------------------------------------------------------" << endl;

    // 1. Print Registers
    cout << " REGISTERS:" << endl;
    cout << "   IC: " << setfill('0') << setw(2) << IC << "       ";
    // Use charArrToString for registers too, to show empty state as '....'
    cout << "   IR: " << charArrToString(IR) << "     ";
    cout << "   R: " << charArrToString(R) << "      ";
    cout << "   C: " << (C ? "T" : "F") << "      ";
    cout << "   SI: " << SI << endl;

    // 2. Print Buffer
    cout << " BUFFER:" << endl;
    cout << "   '" << buffer << "'" << endl;

    // 3. Print Memory (100 Words - Vertically)
    cout << " MEMORY (100 Words):" << endl;
    bool useSeparators = true; // This gets the |G|D|2|0| format

    for (int i = 0; i < 100; i++)
    { // 100 rows
      cout << "  M[" << setfill('0') << setw(2) << i << "]: " << charArrToString(M[i], useSeparators) << endl;
    }
    cout << "=======================================================" << endl;
  }

public:
  OS(string in_file, string out_file);
  ~OS();
  void LOAD();
};

// Constructor
OS::OS(string in_file, string out_file)
{
  infile.open(in_file, ios::in);
  outfile.open(out_file, ios::out);
  if (!infile)
  {
    cout << "Failure: input.txt not found." << endl;
    exit(1);
  }
  cout << "OS Started. Files opened." << endl;
}

// Destructor
OS::~OS()
{
  infile.close();
  outfile.close();
  cout << "\nOS Shutting Down. Files closed." << endl;
}

// init: Initialize all registers and memory
void OS::init()
{
  for (int i = 0; i < 100; i++)
  {
    for (int j = 0; j < 4; j++)
      M[i][j] = ' ';
  }
  for (int i = 0; i < 4; i++)
  {
    IR[i] = ' ';
    R[i] = ' ';
  }
  IC = 0;
  C = false;
  SI = 0;
  job_terminated = false;
  for (int i = 0; i < 40; i++)
    buffer[i] = ' '; // Clear buffer

  printDashboard("Machine Initialized ($AMJ)");
}

// MOS (Master Mode)
void OS::MOS()
{
  if (SI == 1)
  { // READ
    for (int i = 0; i < 40; i++)
      buffer[i] = '\0';
    infile.getline(buffer, 40);

    if (buffer[0] == '$' && buffer[1] == 'E' && buffer[2] == 'N' && buffer[3] == 'D')
    {
      SI = 3;
      MOS();
      job_terminated = true;
      printDashboard("MOS: Out of Data ($END found)");
      return;
    }

    int i = (IR[2] - '0') * 10 + (IR[3] - '0');
    int k = 0;
    for (int l = 0; l < 10; l++)
    {
      for (int j = 0; j < 4; j++)
      {
        if (k >= 40 || buffer[k] == '\0')
          break;
        M[i][j] = buffer[k];
        k++;
      }
      if (k >= 40 || buffer[k] == '\0')
        break;
      i++;
    }
    printDashboard("MOS: READ data card into M[" + to_string((IR[2] - '0') * 10 + (IR[3] - '0')) + "]");
  }
  else if (SI == 2)
  { // WRITE
    for (int i = 0; i < 40; i++)
      buffer[i] = '\0';
    int i = (IR[2] - '0') * 10 + (IR[3] - '0');

    int k = 0;
    for (int l = 0; l < 10; l++)
    {
      for (int j = 0; j < 4; j++)
      {
        if (k >= 40)
          break;
        buffer[k] = M[i][j];
        outfile << buffer[k];
        k++;
      }
      if (k >= 40)
        break;
      i++;
    }
    outfile << "\n";
    printDashboard("MOS: WRITE data from M[" + to_string(i) + "] to output.txt");
  }
  else if (SI == 3)
  { // TERMINATE
    outfile << "\n\n";
    job_terminated = true;
    printDashboard("MOS: TERMINATE (H). Job complete.");
  }
}

// EXECUTEUSERPROGRAM (Slave Mode)
void OS::ExecuteUserProgram()
{
  job_terminated = false;

  while (true)
  {
    // 1. FETCH
    for (int i = 0; i < 4; i++)
    {
      IR[i] = M[IC][i];
    }
    IC++;
    printDashboard("FETCHED Instruction from M[" + to_string(IC - 1) + "]");

    // 2. DECODE
    int i = (IR[2] - '0') * 10 + (IR[3] - '0');

    if (IR[0] == 'G' && IR[1] == 'D')
    { // GD
      SI = 1;
      MOS();
    }
    else if (IR[0] == 'P' && IR[1] == 'D')
    { // PD
      SI = 2;
      MOS();
    }
    else if (IR[0] == 'H')
    { // H
      SI = 3;
      MOS();
      break; // End-Loop
    }
    else if (IR[0] == 'L' && IR[1] == 'R')
    { // LR
      for (int j = 0; j < 4; j++)
      {
        R[j] = M[i][j];
      }
      printDashboard("EXECUTED: LR (Load R from M[" + to_string(i) + "])");
    }
    else if (IR[0] == 'S' && IR[1] == 'R')
    { // SR
      for (int j = 0; j < 4; j++)
      {
        M[i][j] = R[j];
      }
      printDashboard("EXECUTED: SR (Store R into M[" + to_string(i) + "])");
    }
    else if (IR[0] == 'C' && IR[1] == 'R')
    { // CR
      int count = 0;
      for (int j = 0; j < 4; j++)
      {
        if (R[j] == M[i][j])
        {
          count++;
        }
      }
      C = (count == 4);
      printDashboard("EXECUTED: CR (Compare R with M[" + to_string(i) + "]). C = " + (C ? "T" : "F"));
    }
    else if (IR[0] == 'B' && IR[1] == 'T')
    { // BT
      if (C == true)
      {
        IC = i;
      }
      printDashboard(string("EXECUTED: BT. (C is ") + (C ? "True" : "False") + "). IC is now " + to_string(IC));
    }
    else
    {
      SI = 3;
      MOS();
      printDashboard("ERROR: Invalid Opcode. Terminating.");
    }

    if (job_terminated)
    {
      break; // End-Loop
    }
  }
}

// LOAD function
void OS::LOAD()
{
  int m = 0;
  while (infile.getline(buffer, 41))
  {
    string card = buffer;

    if (card.substr(0, 4) == "$AMJ")
    {
      init(); // This will print the first dashboard
      m = 0;
    }
    else if (card.substr(0, 4) == "$DTA")
    {
      IC = 00;
      printDashboard("Program loading complete. Starting Execution.");
      ExecuteUserProgram();
    }
    else if (card.substr(0, 4) == "$END")
    {
      printDashboard("Job Finished ($END found).");
      m = 0;
      continue;
    }
    else
    {
      // This is a Program Card
      if (m >= 100)
      {
        printDashboard("ERROR: Memory Exceeded.");
        break;
      }
      int k = 0;
      for (int i = m; i < m + 10; i++)
      {
        for (int j = 0; j < 4; j++)
        {
          if (k >= 40 || buffer[k] == '\0' || buffer[k] == '\n')
          {
            M[i][j] = ' ';
          }
          else
          {
            M[i][j] = buffer[k];
            k++;
          }
        }
      }
      m += 10;
      printDashboard("Loaded Program Card into M[" + to_string(m - 10) + "]");
    }
  }
}

// Main function
int main()
{
  OS myOS("input1.txt", "output1.txt");
  myOS.LOAD();
  return 0;
}