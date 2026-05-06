#include "sde.h"

// Variable name to index mapping for CIN file parsing
typedef struct {
  const char* name;
  size_t varIndex;
  size_t numSubscripts;
  const char** subscriptNames;
} VariableMapping;

// Variable to index mapping for CSV file parsing for lookups
typedef struct {
  const char* name;
  size_t varIndex;
  size_t numSubscripts;
  const char** subscriptNames;
} LookupVariableMapping;

// Dimension subscript mappings
static const char* member_subscripts[] = {"M1", "M2", "M3", "M4", "M5", "M6", "M7", "M8", "M9", "M10", "M11", "M12", "M13", "M14", "M15", "M16", "M17", "M18", "M19", "M20"};
static const char* bank_subscripts[] = {"B1", "B2", "B3", "B4", "B5"};
static const char* loan_types_subscripts[] = {"L1", "L2", "L3", "L4", "L5"};
static const char* food_groups_subscripts[] = {"Cereals Millets", "Pulses", "Milk", "Roots", "Leafy vegetables", "Vegetables", "Fruits", "Sugar", "Fat"};
static const char* expenditure_subscripts[] = {"Groceries", "Cooked Food", "Education", "House", "Health", "Transport", "Electricity", "Water", "Sanitation", "Waste Collection", "Cooking fuel", "Internet and communication", "Loan Repayment", "Other", "Subsidized item payment", "Appliances", "Insurance premiums", "Repair and maintenance", "Vacations and social functions", "Work", "Agriculture", "College", "Clothing"};
static const char* dsd_subscripts[] = {"D0", "D1", "D2", "D3", "D4", "D5"};
static const char* dws_subscripts[] = {"S1", "S2", "S3", "S4", "S5"};

static const LookupVariableMapping loopkupVariableMappings[] = {
  {"Govt. Transfers", 54, 0, NULL},
  {"Expenditure Shock", 55, 1, NULL},
  {"Income Shock Percentage", 56, 0, NULL},
  {"Nutrition Scheme", 57, 1, NULL},
  {"Subsidy School Fees", 58, 1, NULL},
  {NULL, 0, 0, NULL} // Sentinel
};

// Variable mappings from setConstant function
static const VariableMapping variableMappings[] = {
  {"Education Inflation", 1, 0, NULL},
  {"Income Increase", 2, 0, NULL},
  {"Assets", 3, 0, NULL},
  {"Avg Pregnancy Cost", 4, 0, NULL},
  {"Bank Interest Rate", 5, 1, NULL}, // bank dimension
  {"Borrowing Value", 6, 1, NULL}, // loan_types dimension
  {"Borrowings", 7, 1, NULL}, // loan_types dimension
  {"calorie_content_food", 8, 1, NULL}, // food_groups dimension
  {"Cash Transfer", 9, 0, NULL},
  {"Cost of Food Constants", 10, 1, NULL}, // food_groups dimension
  {"Debt Multiplier", 11, 1, NULL}, // loan_types dimension
  {"deposits", 12, 1, NULL}, // bank dimension
  {"disability_weights_state", 13, 2, NULL}, // member x dws dimensions
  {"disease_hospital_expenditure_demand_constants", 14, 2, NULL}, // member x dws dimensions
  {"disease_medicine_expenditure_demand_constants", 15, 2, NULL}, // member x dws dimensions
  {"disease_state_duration", 16, 2, NULL}, // member x dsd dimensions
  {"education_subsidy", 17, 0, NULL},
  {"Expenditure Constants", 18, 1, NULL}, // expenditure dimension
  {"FINAL TIME", 19, 0, NULL},
  {"Food Exp Demand Multiplier", 20, 0, NULL},
  {"Food Inflation", 21, 0, NULL},
  {"Gender", 22, 1, NULL}, // member dimension
  {"gratuity_multiplier", 23, 0, NULL},
  {"Health Expenditure Demand Constants", 24, 1, NULL}, // member dimension
  {"health_insurance_coverage", 25, 0, NULL},
  {"household_income", 26, 0, NULL},
  {"initial_time", 27, 0, NULL},
  {"Income Quantum", 28, 1, NULL}, // member dimension
  {"increase_in_age", 29, 0, NULL},
  {"Initial Age", 30, 1, NULL}, // member dimension
  {"Initial Bank Savings", 31, 1, NULL}, // bank dimension
  {"Initial Education", 32, 1, NULL}, // member dimension
  {"initial_savings", 33, 0, NULL},
  {"Interest Rate", 34, 1, NULL}, // loan_types dimension
  {"Loan Amount", 35, 0, NULL},
  {"Loan Emi Constants", 36, 1, NULL}, // loan_types dimension
  {"medical_subsidy", 37, 0, NULL},
  {"nutrition_subsidy", 38, 0, NULL},
  {"other_inflation", 39, 0, NULL},
  {"pregnant_member", 40, 1, NULL}, // member dimension
  {"Priority Budget", 41, 1, NULL}, // expenditure dimension
  {"priority_food", 42, 1, NULL}, // food_groups dimension
  {"priority_health", 43, 1, NULL}, // member dimension
  {"priority_loan", 44, 1, NULL}, // loan_types dimension
  {"priority_member", 45, 1, NULL}, // member dimension
  {"rent_subsidy", 46, 0, NULL},
  {"School Fees Constants", 47, 1, NULL}, // member dimension
  {"subsidy_percentage", 48, 1, NULL}, // expenditure dimension
  {"time_step", 49, 0, NULL},
  {"transport_subsidy", 50, 0, NULL},
  {"unemployment_insurance", 51, 0, NULL},
  {"utilities_subsidy", 52, 0, NULL},
  {"width", 53, 0, NULL},
  {"time", 59, 0, NULL},
  {NULL, 0, 0, NULL} // Sentinel
};

/**
 * Get dimension size and subscript names for a given dimension index.
 */
static const char** getDimensionSubscripts(int dimIndex, size_t* dimSize) {
  switch (dimIndex) {
    case 0: // member
      *dimSize = 20;
      return member_subscripts;
    case 1: // bank
      *dimSize = 5;
      return bank_subscripts;
    case 2: // loan_types
      *dimSize = 5;
      return loan_types_subscripts;
    case 3: // food_groups
      *dimSize = 9;
      return food_groups_subscripts;
    case 4: // expenditure
      *dimSize = 23;
      return expenditure_subscripts;
    case 5: // dsd
      *dimSize = 6;
      return dsd_subscripts;
    case 6: // dws
      *dimSize = 5;
      return dws_subscripts;
    default:
      *dimSize = 0;
      return NULL;
  }
}

/**
 * Parse a CIN file in Vensim format and populate constantValues and constantIndices arrays.
 */
static size_t parseCINFile(const char* cinFilePath, double** constantValues, int32_t** constantIndices) {
  FILE* file = fopen(cinFilePath, "r");
  if (!file) {
    fprintf(stderr, "Error: Cannot open CIN file '%s'\n", cinFilePath);
    return 0;
  }

  char line[1024];
  size_t count = 0;
  size_t capacity = 100;
  size_t indicesSize = 1; // Start after count at index 0
  *constantValues = (double*)malloc(capacity * sizeof(double));
  *constantIndices = (int32_t*)malloc((capacity * 4 + 1) * sizeof(int32_t)); // varIndex + subCount + subIndices


  while (fgets(line, sizeof(line), file)) {
    // Remove trailing newline
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') {
      line[len - 1] = '\0';
    }

    // Skip empty lines and comments
    if (line[0] == '\0' || line[0] == '#') {
      continue;
    }

    // Parse line: "Variable[Sub1,Sub2] = value" or "Variable = value"
    char* equals = strchr(line, '=');
    if (!equals) {
      fprintf(stderr, "Warning: Invalid line in CIN file: %s\n", line);
      continue;
    }

    *equals = '\0';
    char* varPart = line;
    char* valuePart = equals + 1;

    // Trim whitespace
    while (*varPart == ' ') varPart++;
    while (*valuePart == ' ') valuePart++;
    while (strlen(varPart) > 0 && varPart[strlen(varPart) - 1] == ' ') {
      varPart[strlen(varPart) - 1] = '\0';
    }
    while (strlen(valuePart) > 0 && valuePart[strlen(valuePart) - 1] == ' ') {
      valuePart[strlen(valuePart) - 1] = '\0';
    }

    // Parse variable name and subscripts
    char varName[256];
    char subNames[10][256]; // Max 10 subscripts
    int numSubs = 0;

    char* bracket = strchr(varPart, '[');
    if (bracket) {
      // Has subscripts
      *bracket = '\0';
      strcpy(varName, varPart);

      char* subPart = bracket + 1;
      char* endBracket = strchr(subPart, ']');
      if (endBracket) {
        *endBracket = '\0';
      }

      // Parse comma-separated subscript names
      
      char* token = strtok(subPart, ",");
      while (token && numSubs < 10) {
        while (*token == ' ') token++;
        // Remove trailing space
        char* end = token + strlen(token) - 1;
        while (end > token && *end == ' ') *end-- = '\0';
        strcpy(subNames[numSubs++], token);
        token = strtok(NULL, ",");
      }
    } else {
      // No subscripts
      strcpy(varName, varPart);
    }

    double value = atof(valuePart);

    // Find variable mapping
    const VariableMapping* mapping = NULL;
    for (size_t i = 0; variableMappings[i].name != NULL; i++) {
      if (strcmp(variableMappings[i].name, varName) == 0) {
        mapping = &variableMappings[i];
        break;
      }
    }

    if (!mapping) {
      fprintf(stderr, "Warning: Unknown variable '%s' in CIN file\n", varName);
      continue;
    }

    // Validate subscripts
    if (mapping->numSubscripts != (size_t)numSubs) {
      fprintf(stderr, "Warning: Variable '%s' expects %zu subscripts, got %d\n", varName, mapping->numSubscripts, numSubs);
      continue;
    }

    // Convert subscript names to indices
    int subIndices[10];
    bool validSubs = true;
    for (size_t i = 0; i < mapping->numSubscripts; i++) {
      int dimIndex;
      if (mapping->numSubscripts == 1) {
        // 1D arrays: determine dimension from variable
        if (strcmp(mapping->name, "Bank Interest Rate") == 0 || strcmp(mapping->name, "Deposits") == 0 || strcmp(mapping->name, "Initial Bank Savings") == 0) {
          dimIndex = 1; // bank
        } else if (strcmp(mapping->name, "Borrowing Value") == 0 || strcmp(mapping->name, "Borrowings") == 0 || strcmp(mapping->name, "Debt Multiplier") == 0 ||
                   strcmp(mapping->name, "Interest Rate") == 0 || strcmp(mapping->name, "Loan Emi Constants") == 0 || strcmp(mapping->name, "Priority Loan") == 0) {
          dimIndex = 2; // loan_types
        } else if (strcmp(mapping->name, "calorie_content_food") == 0 || strcmp(mapping->name, "Cost of Food Constants") == 0 || strcmp(mapping->name, "priority_food") == 0) {
          dimIndex = 3; // food_groups
        } else if (strcmp(mapping->name, "Expenditure Constants") == 0 || strcmp(mapping->name, "Priority Budget") == 0 || strcmp(mapping->name, "subsidy_percentage") == 0) {
          dimIndex = 4; // expenditure
        } else {
          dimIndex = 0; // member (default for most 1D arrays)
        }
      } else {
        // 2D arrays: first dim is member, second dim depends on variable
        if (i == 0) {
          dimIndex = 0; // member
        } else {
          if (strcmp(mapping->name, "disease_state_duration") == 0) {
            dimIndex = 5; // dsd
          } else {
            dimIndex = 6; // dws (for disability_weights_state and disease expenditure constants)
          }
        }
      }

      size_t dimSize;
      const char** dimSubs = getDimensionSubscripts(dimIndex, &dimSize);
      if (!dimSubs) {
        fprintf(stderr, "Warning: Unknown dimension %d for variable '%s'\n", dimIndex, varName);
        validSubs = false;
        break;
      }
      int idx = -1;
      for (size_t j = 0; j < dimSize; j++) {
        if (strcmp(dimSubs[j], subNames[i]) == 0) {
          idx = (int)j;
          break;
        }
      }
      if (idx == -1) {
        fprintf(stderr, "Warning: Unknown subscript '%s' for dimension %d of variable '%s'\n", subNames[i], dimIndex, varName);
        validSubs = false;
        break;
      }
      subIndices[i] = idx;
    }

    if (!validSubs) {
      continue;
    }

    // Grow arrays if needed
    if (count >= capacity) {
      capacity *= 2;
      *constantValues = (double*)realloc(*constantValues, capacity * sizeof(double));
      *constantIndices = (int32_t*)realloc(*constantIndices, (capacity * 4 + 1) * sizeof(int32_t));
    }

    // Add to buffers at the current write position
    (*constantIndices)[indicesSize] = (int32_t)mapping->varIndex;
    (*constantIndices)[indicesSize + 1] = (int32_t)mapping->numSubscripts;

    for (size_t i = 0; i < mapping->numSubscripts; i++) {
      (*constantIndices)[indicesSize + 2 + i] = subIndices[i];
    }
    (*constantValues)[count] = value;
    
    // Increment write position by: varIndex + numSubscripts + subscript indices
    indicesSize += 2 + mapping->numSubscripts;

    count++;
  }



  fclose(file);

  // Set count at index 0
  (*constantIndices)[0] = (int32_t)count;

  return count;
}

// Parse a CSV file containing lookup data and convert it into
// two flat buffers:
//   1. lookupValues  -> actual (x, y) pairs
//   2. lookupIndices -> metadata describing where each lookup belongs
static size_t parseLookupCSV(
  const char* filePath,     // path to CSV file
  double** lookupValues,    // output: array of lookup values
  int32_t** lookupIndices   // output: array of lookup metadata
) {

  // Open the CSV file for reading
  FILE* file = fopen(filePath, "r");
  if (!file) {
    fprintf(stderr, "Error opening lookup CSV\n");
    return 0; // fail early if file can't be opened
  }

  // Number of lookup entries parsed (one per line)
  size_t count = 0;

  // Initial capacities for dynamic arrays
  size_t valCapacity = 1000;   // number of doubles
  size_t idxCapacity = 200;   // number of int32s

  // Allocate initial memory
  *lookupValues = malloc(valCapacity * sizeof(double));
  *lookupIndices = malloc(idxCapacity * sizeof(int32_t));

  // Offsets track where we are writing in the arrays
  size_t valOffset = 0;   // current write position in lookupValues
  size_t idxOffset = 1;   // reserve index[0] to store count later

  char line[1024]; // buffer to read each CSV line

  // Read file line-by-line
  while (fgets(line, sizeof(line), file)) {

    // Skip empty lines or comments
    if (line[0] == '\0' || line[0] == '#') continue;

    // Remove newline character at end of line
    line[strcspn(line, "\n")] = '\0';

    // Define pointer for line tokens
    char *lineTokenPtr;

    // First token: "Variable[subs]"
    char* token = strtok_r(line, ",", &lineTokenPtr);
    if (!token) continue; // skip malformed lines

    // -----------------------------
    // PARSE VARIABLE + SUBSCRIPTS
    // -----------------------------

    char varName[256];         // variable name
    char subNames[10][256];    // subscript names (max 10 dims)
    int numSubs = 0;           // number of subscripts found

    // Check if variable has subscripts (e.g., Var[A,B])
    char* bracket = strchr(token, '[');

    if (bracket) {
      // Split variable name and subscript part
      *bracket = '\0';
      strcpy(varName, token); // copy variable name

      char* subPart = bracket + 1;

      // Remove closing bracket
      char* end = strchr(subPart, ']');
      if (end) *end = '\0';

      // Define pointer for variable sub tokens
      char *subTokensPtr;

      // Parse comma-separated subscript names
      char* subTok = strtok_r(subPart, ",", &subTokensPtr);
      while (subTok) {

        // Trim leading spaces
        while (*subTok == ' ') subTok++;

        // Trim trailing spaces
        char* end = subTok + strlen(subTok) - 1;
        while (end > subTok && *end == ' ') *end-- = '\0';

        // Store cleaned subscript
        strcpy(subNames[numSubs++], subTok);

        subTok = strtok_r(NULL, ",", &subTokensPtr);
      }

    } else {
      // No subscripts → just copy variable name
      strcpy(varName, token);
    }

    // -----------------------------
    // FIND VARIABLE MAPPING
    // -----------------------------

    const LookupVariableMapping* mapping = NULL;

    // Loop through known lookup variables
    for (int i = 0; loopkupVariableMappings[i].name; i++) {
      if (strcmp(loopkupVariableMappings[i].name, varName) == 0) {
        mapping = &loopkupVariableMappings[i];
        break;
      }
    }

    // If variable is not recognized, skip it
    if (!mapping) {
      fprintf(stderr, "Unknown lookup variable '%s'\n", varName);
      continue;
    }

    // -----------------------------
    // CONVERT SUBSCRIPT NAMES → INDICES
    // -----------------------------

    size_t subIndicesLocal[10]; // store numeric indices
    bool validSubs = true;

    for (size_t i = 0; i < mapping->numSubscripts; i++) {

      int dimIndex;

      // Determine which dimension this variable belongs to
      if (mapping->numSubscripts == 1) {
        if (strcmp(mapping->name, "Expenditure Shock") == 0) {
          dimIndex = 4; // expenditure dimension
        } else if (strcmp(mapping->name, "Subsidy School Fees") == 0) {
          dimIndex = 0; // member dimension
        } else if (strcmp(mapping->name, "Nutrition Scheme") == 0) {
          dimIndex = 3; // food groups dimension
        } else {
          dimIndex = 0; // fallback default
        }
      } else {
        dimIndex = 0; // future extension for multi-dim lookups
      }

      // Get valid subscript names for that dimension
      size_t dimSize;
      const char** dimSubs = getDimensionSubscripts(dimIndex, &dimSize);

      int idx = -1;

      // Find matching subscript index
      for (size_t j = 0; j < dimSize; j++) {
        if (strcmp(dimSubs[j], subNames[i]) == 0) {
          idx = (int)j;
          break;
        }
      }

      // If not found → invalid input
      if (idx == -1) {
        fprintf(stderr, "Invalid subscript '%s'\n", subNames[i]);
        validSubs = false;
        break;
      }

      subIndicesLocal[i] = (size_t)idx;

    }

    // Skip this row if subscripts invalid
    if (!validSubs) {
      continue;
    }

    // -----------------------------
    // READ LOOKUP POINTS
    // -----------------------------

    size_t numPoints = 0;

    // Temporary buffer to hold raw values from CSV
    double tempPoints[100000];

    // Read remaining tokens (values)
    while ((token = strtok_r(NULL, ",", &lineTokenPtr))) {
      tempPoints[numPoints++] = atof(token);
    }

    // Safety check
    if (numPoints >= 50000) {
      fprintf(stderr, "Too many lookup points (max 50000)\n");
      break;
    }

    // -----------------------------
    // ENSURE BUFFER CAPACITY
    // -----------------------------

    // Grow values buffer if needed
    while (valOffset + numPoints >= valCapacity) {
      valCapacity *= 2;
    }
    *lookupValues = realloc(*lookupValues, valCapacity * sizeof(double));

    // Grow indices buffer if needed
    while (idxOffset + 3 + mapping->numSubscripts >= idxCapacity) {
      idxCapacity *= 2;
    }
    *lookupIndices = realloc(*lookupIndices, idxCapacity * sizeof(int32_t));

    // -----------------------------
    // WRITE METADATA (INDICES ARRAY)
    // -----------------------------

    // Format per lookup:
    // [varIndex, numSubs, subIndices..., numPoints]

    (*lookupIndices)[idxOffset++] = (int32_t)mapping->varIndex;
    (*lookupIndices)[idxOffset++] = (int32_t)mapping->numSubscripts;

    // Write subscript indices
    for (size_t i = 0; i < mapping->numSubscripts; i++) {
      (*lookupIndices)[idxOffset++] = (int32_t)subIndicesLocal[i];
    }

    // Store number of lookup values
    (*lookupIndices)[idxOffset++] = (int32_t)numPoints;

    // -----------------------------
    // WRITE LOOKUP VALUES
    // -----------------------------

    // You are constructing (x, y) pairs like:
    // (0, val0), (1, val1), (2, val2), ...
    // i.e., index becomes x-axis

    for (size_t i = 0; i < numPoints; i++) {
      (*lookupValues)[valOffset++] = i;                // x value
      (*lookupValues)[valOffset++] = tempPoints[i];    // y value
    }

    // One lookup parsed successfully
    count++;
  }

  fclose(file);

  // Store total number of lookup entries at index 0
  (*lookupIndices)[0] = (int32_t)count;

  return count;
}
static size_t countInputs(const char* inputData) {
  if (inputData == NULL || *inputData == '\0') {
    return 0;
  }

  // Make a copy since strtok modifies the string
  char* inputsCopy = (char*)malloc(strlen(inputData) + 1);
  strcpy(inputsCopy, inputData);

  size_t count = 0;
  char* token = strtok(inputsCopy, " ");
  while (token) {
    if (strchr(token, ':') != NULL) {
      count++;
    }
    token = strtok(NULL, " ");
  }
  free(inputsCopy);

  return count;
}

/**
 * Parse an input data string in the format "varIndex:value varIndex:value ..."
 * (for example, "0:3.14 6:42") and populate the inputValues and inputIndices
 * arrays for sparse input setting.
 *
 * @param inputData The input string to parse.
 * @param inputValues The array to populate with input values.
 * @param inputIndices The array to populate with input indices (first element is count).
 */
static void parseInputs(const char* inputData, double* inputValues, int32_t* inputIndices) {
  if (inputData == NULL || *inputData == '\0') {
    return;
  }

  // Make a copy since strtok modifies the string
  char* inputsCopy = (char*)malloc(strlen(inputData) + 1);
  strcpy(inputsCopy, inputData);

  size_t i = 0;
  char* token = strtok(inputsCopy, " ");
  while (token) {
    char* p = strchr(token, ':');
    if (p) {
      *p = '\0';
      int modelVarIndex = atoi(token);
      double value = atof(p + 1);
      inputIndices[i + 1] = modelVarIndex;
      inputValues[i] = value;
      i++;
    }
    token = strtok(NULL, " ");
  }
  inputIndices[0] = (int32_t)i;
  free(inputsCopy);
}

int main(int argc, char** argv) {
  // TODO make the input buffer size dynamic
  char inputString[500000];
  char cinFilePath[500000] = ""; // Path to CIN file
  char dataFilePath[500000] = ""; // Path to cvs data file
  // When true, output data without newlines or a header, suitable for embedding reference data.
  bool raw_output = false;
  // When true, suppress data output when using PR* macros.
  bool suppress_data_output = false;
  // Try to read input from a file named in the argument.
  if (argc > 1) {
    FILE* instream = fopen(argv[1], "r");
    if (instream && fgets(inputString, sizeof inputString, instream) != NULL) {
      fclose(instream);
      size_t len = strlen(inputString);
      if (inputString[len - 1] == '\n') {
        inputString[len - 1] = '\0';
      }
    }
  } else {
    *inputString = '\0';
  }

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--cin") == 0 && i + 1 < argc) {
      strcpy(cinFilePath, argv[++i]);
    } else if (strcmp(argv[i], "--data") == 0 && i + 1 < argc) {
      strcpy(dataFilePath, argv[++i]);
    } else if (strcmp(argv[i], "--raw") == 0) {
      raw_output = true;
    }
  }

  // Parse input string and create sparse input arrays. Only allocate buffers if there
  // are inputs to parse; otherwise pass NULL to runModelWithBuffers so that the model
  // uses its default values from initConstants.
  double* inputValues = NULL;
  int32_t* inputIndices = NULL;
  size_t inputCount = countInputs(inputString);
  if (inputCount > 0) {
    inputValues = (double*)malloc(inputCount * sizeof(double));
    inputIndices = (int32_t*)malloc((inputCount + 1) * sizeof(int32_t));
    parseInputs(inputString, inputValues, inputIndices);
  }

  // Parse CIN file and create sparse constant arrays
  double* constantValues = NULL;
  int32_t* constantIndices = NULL;
  size_t constantCount = 0;

  if (cinFilePath[0] != '\0') {
    constantCount = parseCINFile(cinFilePath, &constantValues, &constantIndices);
    if (constantCount == 0) {
      fprintf(stderr, "Warning: No constants parsed from CIN file '%s'\n", cinFilePath);
    }
  }

  // Parse lookup CSV
  double* lookupValues = NULL;
  int32_t* lookupIndices = NULL;

  if (dataFilePath[0] != '\0') {
    size_t lookupCount = parseLookupCSV(dataFilePath, &lookupValues, &lookupIndices);
    if (lookupCount == 0) {
      fprintf(stderr, "No lookup entries parsed\n");
    }
  }


  // Calculate the number of save points for the output buffer
  double initialTime = getInitialTime();
  double finalTime = getFinalTime();
  double saveper = getSaveper();
  size_t numSavePoints = (size_t)(round((finalTime - initialTime) / saveper)) + 1;

  // Allocate output buffer
  double* outputBuffer = (double*)malloc(numOutputs * numSavePoints * sizeof(double));

  // Run the model with the sparse input arrays and output buffer
  // runModelWithBuffers(inputValues, inputIndices, outputBuffer, NULL, constantValues, constantIndices);
  runModelWithBuffers(inputValues, inputIndices, outputBuffer, NULL, constantValues, constantIndices, lookupValues, lookupIndices);

  if (!suppress_data_output) {
    if (raw_output) {
      // Write raw output data directly (tab-separated, no newlines)
      for (size_t t = 0; t < numSavePoints; t++) {
        for (size_t v = 0; v < numOutputs; v++) {
          // Output buffer is organized by variable (each variable has numSavePoints values)
          double value = outputBuffer[v * numSavePoints + t];
          printf("%g\t", value);
        }
      }
    } else {
      // Write a header for output data.
      printf("%s\n", getHeader());
      // Write tab-delimited output data, one line per output time step.
      for (size_t t = 0; t < numSavePoints; t++) {
        for (size_t v = 0; v < numOutputs; v++) {
          // Output buffer is organized by variable (each variable has numSavePoints values)
          double value = outputBuffer[v * numSavePoints + t];
          if (v > 0) {
            printf("\t");
          }
          printf("%g", value);
        }
        printf("\n");
      }
    }
  }

  // Clean up
  if (inputValues != NULL) {
    free(inputValues);
  }
  if (inputIndices != NULL) {
    free(inputIndices);
  }
  if (constantValues != NULL) {
    free(constantValues);
  }
  if (constantIndices != NULL) {
    free(constantIndices);
  }
  if (lookupValues != NULL) {
    free(lookupValues);
  }
  if (lookupIndices != NULL) {
    free(lookupIndices);
  }
  free(outputBuffer);
  finish();
}
