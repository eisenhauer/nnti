#include "EpetraExt_ConfigDefs.h"
#ifdef HAVE_MPI
#include "Epetra_MpiComm.h"
#include "mpi.h"
#else
#include "Epetra_SerialComm.h"
#endif
#include "EpetraExt_XMLReader.h"
#include "Teuchos_ParameterList.hpp"
#include "Teuchos_RefCountPtr.hpp"
#include "Teuchos_XMLObject.hpp"
#include "Teuchos_StringInputSource.hpp"
#include "Teuchos_FileInputSource.hpp"
#include "Teuchos_ParameterList.hpp"
#include "Teuchos_XMLParameterListReader.hpp"
#include "Teuchos_TestForException.hpp"
#include "Epetra_Map.h"
#include "Epetra_CrsGraph.h"
#include "Epetra_FECrsGraph.h"
#include "Epetra_RowMatrix.h"
#include "Epetra_CrsMatrix.h"
#include "Epetra_FECrsMatrix.h"
#include "Epetra_MultiVector.h"
#include "Epetra_Import.h"

// ============================================================================
static void Tokenize(const string& str, vector<string>& tokens,
              const string& delimiters = " ")
{
  // Skip delimiters at beginning.
  string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  string::size_type pos     = str.find_first_of(delimiters, lastPos);

  while (string::npos != pos || string::npos != lastPos)
  {
    // Found a token, add it to the vector.
    tokens.push_back(str.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = str.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = str.find_first_of(delimiters, lastPos);
  }
}
using namespace Teuchos;

// ============================================================================
EpetraExt::XMLReader::XMLReader(const Epetra_Comm& comm, const string& FileName) :
  Comm_(comm)
{
#ifdef HAVE_TEUCHOS_EXPAT
  FileInputSource fileSrc(FileName);
  fileXML_ = rcp(new XMLObject(fileSrc.getObject()));
  IsOpen_ = true;
#else
  cerr << "Teuchos was not configured with support for expat." << endl;
  cerr << "Please reconfigure teuchos with --enable-teuchos-expat." << endl;
  exit(EXIT_FAILURE);
#endif
}

// ============================================================================
void EpetraExt::XMLReader::
Read(const string& Label, Epetra_CrsGraph*& Graph)
{
  TEST_FOR_EXCEPTION(IsOpen_ == false, std::logic_error,
                     "No file has been opened");

  Graph = 0;

  for (int i = 0; i < fileXML_->numChildren(); ++i)
  {
    const XMLObject& child = fileXML_->getChild(i);
    string tag = child.getTag();

    if (tag == "Graph")
    {
      if (child.hasAttribute("Label") && child.getRequired("Label") == Label)
      {
        int NumGlobalRows = child.getRequiredInt("Rows");
        int NumGlobalCols = child.getRequiredInt("Columns");
        int NumGlobalEntries = child.getRequiredInt("Entries");
        int Offset = child.getRequiredInt("StartingIndex");

        Epetra_Map map(NumGlobalRows, 0, Comm_);
        Graph = new Epetra_CrsGraph(Copy, map, 0);

        for (int j = 0; j < child.numContentLines(); ++j)
        {
          vector<string> tokens;
          const string& line = child.getContentLine(j);
          Tokenize(line, tokens, " \n\r\t");
          if (tokens.size() < 2) continue;

          int row, col;
          row = atoi((char*)tokens[0].c_str());
          col = atoi((char*)tokens[1].c_str());

          if (map.LID(row) != -1)
            Graph->InsertGlobalIndices(row, 1, &col);
        }
        Graph->FillComplete();
      }
    }
  }
}

// ============================================================================
void EpetraExt::XMLReader::
Read(const string& Label, Epetra_CrsMatrix*& matrix)
{
  TEST_FOR_EXCEPTION(IsOpen_ == false, std::logic_error,
                     "No file has been opened");

  matrix = 0;

  for (int i = 0; i < fileXML_->numChildren(); ++i)
  {
    const XMLObject& child = fileXML_->getChild(i);
    string tag = child.getTag();

    if (tag == "PointMatrix")
    {
      if (child.hasAttribute("Label") && child.getRequired("Label") == Label)
      {
        int NumGlobalRows = child.getRequiredInt("Rows");
        int NumGlobalCols = child.getRequiredInt("Columns");
        int NumGlobalNonzeros = child.getRequiredInt("Nonzeros");
        int Offset = child.getRequiredInt("StartingIndex");

        Epetra_Map map(NumGlobalRows, 0, Comm_);
        matrix = new Epetra_CrsMatrix(Copy, map, 0);

        for (int j = 0; j < child.numContentLines(); ++j)
        {
          vector<string> tokens;
          const string& line = child.getContentLine(j);
          Tokenize(line, tokens, " \n\r\t");
          if (tokens.size() < 3) continue;

          int row, col;
          double val;
          row = atoi((char*)tokens[0].c_str());
          col = atoi((char*)tokens[1].c_str());
          val = atof((char*)tokens[2].c_str());

          if (map.LID(row) != -1)
            matrix->InsertGlobalValues(row, 1, &val, &col);
        }
        matrix->FillComplete();
      }
    }
  }
}

// ============================================================================
void EpetraExt::XMLReader::
Read(const string& Label, Epetra_MultiVector*& MultiVector)
{
  TEST_FOR_EXCEPTION(IsOpen_ == false, std::logic_error,
                     "No file has been opened");

  MultiVector = 0;

  // read all file and create all objects in memory.
  for (int i = 0; i < fileXML_->numChildren(); ++i)
  {
    const XMLObject& child = fileXML_->getChild(i);
    string tag = child.getTag();

    if (tag == "MultiVector")
    {
      if (child.hasAttribute("Label") && child.getRequired("Label") == Label)
      {
        int GlobalLength = child.getRequiredInt("Length");
        int NumVectors = child.getRequiredInt("NumVectors");

        Epetra_Map Map(GlobalLength, 0, Comm_);
        MultiVector = new Epetra_MultiVector(Map, NumVectors);

        int count = 0;
        int v = 0;
        double val;
        for (int j = 0; j < child.numContentLines(); ++j)
        {
          vector<string> tokens;

          const string& line = child.getContentLine(j);

          Tokenize(line, tokens, " \n\r\t");

          if (tokens.size() == 0) continue;

          TEST_FOR_EXCEPTION(tokens.size() != NumVectors, std::logic_error,
                             "wrong number of tokens in line; "
                             << "tokens.size() = " << tokens.size() 
                             << ", NumVectors = " << NumVectors);

          for (int k = 0; k < tokens.size(); ++k)
          {
            if (Map.LID(count) != -1)
            {
              sscanf((char*)(tokens[k].c_str()), "%lf", &val);

              (*MultiVector)[k][Map.LID(count)] = val;
            }
          }
          ++count;
        }
      }
    }
  }
}

// ============================================================================
void EpetraExt::XMLReader::
Read(const string& Label, Epetra_Map*& Map)
{
  TEST_FOR_EXCEPTION(IsOpen_ == false, std::logic_error,
                     "No file has been opened");

  Map = 0;

  // read all file and create all objects in memory.
  for (int i = 0; i < fileXML_->numChildren(); ++i)
  {
    const XMLObject& child = fileXML_->getChild(i);
    string tag = child.getTag();

    if (tag == "Map")
    {
      if (child.hasAttribute("Label") && child.getRequired("Label") == Label)
      {
        int NumGlobalElements = child.getRequiredInt("NumElements");
        int IndexBase = child.getRequiredInt("IndexBase");
        int NumProc = child.getRequiredInt("NumProc");

        TEST_FOR_EXCEPTION(NumProc != Comm_.NumProc(), std::logic_error,
                           "Requested map defined with different number of processors, "
                           << "NumProc = " << NumProc << " while "
                           << "Comm.NumProc() = " << Comm_.NumProc());

        char str[80];
        sprintf(str, "ElementsOnProc%d", Comm_.MyPID());
        int NumMyElements = child.getRequiredInt(str);

        sprintf(str, "ElementsOnProc%d", Comm_.MyPID());

        vector<int> MyGlobalElements(NumMyElements);

        for (int iproc = 0; iproc < child.numChildren(); ++iproc)
        {
          const XMLObject& newChild = child.getChild(iproc);
          int count = 0;

          if (newChild.hasAttribute("ID") && 
              newChild.getRequiredInt("ID") == Comm_.MyPID())
          {
            for (int j = 0; j < newChild.numContentLines(); ++j)
            {
              vector<string> tokens;

              const string& line = newChild.getContentLine(j);

              Tokenize(line, tokens, " \n\r\t");

              for (int k = 0; k < tokens.size(); ++k)
              {
                MyGlobalElements[count++] = atoi((char*)tokens[k].c_str());
              }
            }
          }
        }

        Map = new Epetra_Map(NumGlobalElements, NumMyElements,
                             &MyGlobalElements[0], IndexBase, Comm_);
      }
    }
  }
}

// ============================================================================
void EpetraExt::XMLReader::
Read(const string& Label, vector<string>& Content)
{
  TEST_FOR_EXCEPTION(IsOpen_ == false, std::logic_error,
                     "No file has been opened");

  for (int i = 0; i < fileXML_->numChildren(); ++i)
  {
    const XMLObject& child = fileXML_->getChild(i);
    string tag = child.getTag();

    if (tag == "Text")
    {
      if (child.hasAttribute("Label") && child.getRequired("Label") == Label)
      {
        for (int j = 0; j < child.numContentLines(); ++j)
        {
          const string& line = child.getContentLine(j);
          if (line == "\n") continue;
          Content.push_back(line);
        }
      }
    }
  }
}

// ============================================================================
void EpetraExt::XMLReader::
Read(const string& Label, Teuchos::ParameterList& List)
{
  TEST_FOR_EXCEPTION(IsOpen_ == false, std::logic_error,
                     "No file has been opened");

  for (int i = 0; i < fileXML_->numChildren(); ++i)
  {
    const XMLObject& child = fileXML_->getChild(i);
    string tag = child.getTag();

    if (tag == "List")
    {
      if (child.hasAttribute("Label") && child.getRequired("Label") == Label)
      {
        Teuchos::XMLParameterListReader ListReader;
        List = ListReader.toParameterList(child.getChild(0));
      }
    }
  }
}
