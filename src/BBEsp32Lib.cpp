#include "BBEsp32Lib.h"
#include <freertos/task.h>
#include <rom/rtc.h>
#include <dirent.h>
#include <ezTime.h>

#include <RemoteDebug.h>
using namespace std;

extern RemoteDebug Debug;

void taskState(TaskHandle_t task) {
  //eTaskState   ts = eTaskGetState(task);
  //  debugI("%s: state:%l hwm:%l rtc:%l", ts.pcTaskName, (long) ts.eCurrentState,
  //       (long) ts.usStackHighWaterMark, (long)ts.ulRunTimeCounter);
}

/**
   Return "startWith + from + endWith" without any repeated substrings
*/
String slash(const String& startWith, const String&from , const String& endWith) {

  String r = from;

  if (r.startsWith(ps)) {
    r = r.substring(ps.length());
    if (r.startsWith(ps)) {
      r = r.substring(ps.length());
    }
  }

  if (r.endsWith(ps)) {
    r = r.substring(0, r.length() - ps.length());
    if (r.endsWith(ps)) {
      r = r.substring(0, r.length() - ps.length());
    }
  }
  return startWith + r + endWith;
}

/**

*/

String  slash(char startWith, const String&from) {
  String r = from;

  if (r.startsWith(ps)) {
    r = r.substring(ps.length());
    if (r.startsWith(ps)) {
      r = r.substring(ps.length());
    }
  }

  return String(startWith)  + r;
}


String slash(const String&from, char endWith) {
  String r = from;

  if (r.endsWith(ps)) {
    r = r.substring(0, r.length() - ps.length());
    if (r.endsWith(ps)) {
      r = r.substring(0, r.length() - ps.length());
    }
  }
  return r + String(endWith);
}

/**
   ls with chars

  bool ls(char* filenames[],const String& localDir, int maxFiles, const String& endingWith, const String& localFilePrefix) {
  return true;
  }delete
*/

/**
    Use posix functions - much faster than Arduino File class for listDir
*/
std::vector <String> ls(const String& localDir, int maxFiles, int* nTotalFiles, const String& endingWith, const String& localFilePrefix ) {

  std::vector <String> r;
  const int isFile = 1;   // Not portable!
  DIR *folder;
  struct dirent *entry;
  int files = 0;
  int niles = 0;
  String localDirPosix;
  String filename;
  String localDirSlashed ;

  if (localDir != "") {
    localDirSlashed = slash("", localDir, "/");
  }
  localDirPosix = localFilePrefix + slash("", localDir, "");
  folder = opendir(localDirPosix.c_str());

  if (!folder) {
    debugE("ls: couldn't open directory: %s", localDirPosix.c_str());
    return r;
  }
  /*
    Serial.println("------BEFORE---------");
    for (size_t i = 0; i < r.size(); ++i)  {
    Serial.println(r[i]);
    }
    Serial.println("-------------------");
    Serial.println(maxFiles);
    Serial.println(files);
  */

  while  ( (entry = readdir(folder)))   {
    if ( entry->d_type == isFile && entry->d_name) {
      filename =  String(entry->d_name);
      if (filename != "" && filename.endsWith(endingWith)) {
        files++;
        if (files < maxFiles) {
          //Serial.println("<");Serial.println(filename);
          r.push_back(filename);
        } else if (files == maxFiles) {
          Serial.println("=="); Serial.println(filename);
          std::sort(r.begin(), r.end());
          r.push_back(filename);
        } else if (files > maxFiles) {
          //Serial.println(">");Serial.println(filename);
          //std::vector<String>::iterator it = std::upper_bound( r.begin(), r.end(), filename); // find proper position in ascending
          //Serial.println(*it);
          std::vector<String>::iterator i;
          for (i = r.begin(); i != r.end() && (*i) < filename; ++i) {
            //Serial.print(".");
          }
          /*
            Serial.println("");
            if (i == r.end()) {
             Serial.println("Insert at end");
            } else {
             Serial.println("Insert before " + (*i));
            }
          */
          if (i != r.end()) {
            r.insert( i, filename); // insert before iterator it
            r.pop_back();
          }
        } else {
          assert(0);
        }
      } else {
        niles++;
      }
    }
    *nTotalFiles = files;
  }
  /*
    Serial.println("------AFTER---------");
    for (size_t i = 0; i < r.size(); ++i)  {
    Serial.println(r[i]);
    }
    Serial.println("-------------------");
    debugV("Files: %d Niles: %d", files, niles);
  */
  closedir(folder);
  return r;
}

// function to round the number
unsigned long mround_ul(unsigned long  n, int m) {
  // Smaller multiple
  int a = (n / m) * m;
  // Larger multiple
  int b = a + m;
  // Return of closest of two
  return (n - a > b - n) ? b : a;
}


LogFileClass::LogFileClass(const char* _filename):
  filename(_filename) {
}

//void LogFileClass::print(const char* message) {
// print(String(message));

//}

void LogFileClass::print(const String& message) {
  FILE *f ;
  String t;

  if (defaultTZ->year() > 2000) {
    t = defaultTZ->dateTime("d-M-y H:i:s");
  } else {
    t = String(millis()) + String ("ms");
  }
  if ((f = fopen(filename.c_str(), "a"))) {
    fprintf(f, "%s: ", t.c_str());
    fputs(message.c_str(), f);
    fputs("\n", f);
    fclose(f); f = NULL;
  } else {
    Serial.printf("LogToFile: couldn't open %s\n", filename.c_str());
  }
}

void LogFileClass::read() {

  FILE *fp;
  const int MAXCHAR = 256;
  char str[MAXCHAR];

  fp = fopen(filename.c_str(), "r");
  if (fp == NULL) {
    Serial.printf("Could not open log: %s\n", filename.c_str());
    return;
  }
  while (fgets(str, MAXCHAR, fp) != NULL) {
    Serial.printf("%s", str);
  }
  fclose(fp);
  if (ferror(fp)) {
    Serial.printf("Error reading log: %s\n", filename.c_str());
    perror(filename.c_str());
  }
}

String LogFileClass::tail(int lines) {
  int count = 0;  // To count '\n' characters
  FILE* f;
  if (!(f = fopen(filename.c_str(), "r"))) {
    return "";
  }
  const int MAXLINE =   256;
  unsigned long long pos;
  char str[MAXLINE];
  String r;

  // Go to End of file
  if (fseek(f, 0, SEEK_END)) {
    perror("fseek() failed");
  } else  {
    // pos will contain no. of chars in    // input file.
    pos = ftell(f);
    // search for '\n' characters
    while (pos)  {
      // Move 'pos' away from end of file.
      if (!fseek(f, --pos, SEEK_SET))      {
        if (fgetc(f) == '\n') {
          // stop reading when n newlinesis found
          if (count++ == lines)
            break;
        }
      } else {
        perror("fseek() failed");
      }
    }
    while (fgets(str, sizeof(str), f)) {
      r += str;
    }
  }
  r += "\n";
  fclose(f); f = NULL;
  return r;
}

String LogFileClass::head(int lines) {
  int count = 0;  // To count '\n' characters
  FILE* f;
  if (!(f = fopen(filename.c_str(), "r"))) {
    return "";
  }
  const int MAXLINE =   256;
  char str[MAXLINE];
  String r;

  while (fgets(str, sizeof(str), f) && (count++ < lines)) {
    r += str;
  }
  r += "\n";
  fclose(f); f = NULL;
  return r;
}

String resetReason(int core)
{
  assert(core == 1 || core == 0);
  switch ( rtc_get_reset_reason(core))
  {
    case 1 : return String("POWERON_RESET");        /**<1, Vbat power on reset*/
    case 3 : return String("SW_RESET");              /**<3, Software reset digital core*/
    case 4 : return String ("OWDT_RESET");           /**<4, Legacy watch dog reset digital core*/
    case 5 : return String ("DEEPSLEEP_RESET");        /**<5, Deep Sleep reset digital core*/
    case 6 : return String ("SDIO_RESET");           /**<6, Reset by SLC module, reset digital core*/
    case 7 : return String ("TG0WDT_SYS_RESET");       /**<7, Timer Group0 Watch dog reset digital core*/
    case 8 : return String ("TG1WDT_SYS_RESET"); ;      /**<8, Timer Group1 Watch dog reset digital core*/
    case 9 : return String ("RTCWDT_SYS_RESET");       /**<9, RTC Watch dog Reset digital core*/
    case 10 : return String ("INTRUSION_RESET");       /**<10, Instrusion tested to reset CPU*/
    case 11 : return String ("TGWDT_CPU_RESET");       /**<11, Time Group reset CPU*/
    case 12 : return String ("SW_CPU_RESET");          /**<12, Software reset CPU*/
    case 13 : return String ("RTCWDT_CPU_RESET");      /**<13, RTC Watch dog Reset CPU*/
    case 14 : return String ("EXT_CPU_RESET");         /**<14, for APP CPU, reseted by PRO CPU*/
    case 15 : return String ("RTCWDT_BROWN_OUT_RESET");  /**<15, Reset when the vdd voltage is not stable*/
    case 16 : return String ("RTCWDT_RTC_RESET");      /**<16, RTC Watch dog reset digital core and rtc module*/
    default : return String ("NO_MEAN");
  }
}

// This example demonstrates how a human readable table of run time stats
// information is generated from raw data provided by uxTaskGetSystemState().
// The human readable table is written to pcWriteBuffer
void vTaskPrintRunTimeStats() {

  TaskStatus_t *pxTaskStatusArray;
  volatile UBaseType_t uxArraySize, x;
  uint32_t ulTotalRunTime, ulStatsAsPercentage;
  eTaskState state;
  UBaseType_t currentPriority;

  // Take a snapshot of the number of tasks in case it changes while this
  // function is executing.
  uxArraySize = uxTaskGetNumberOfTasks();

  // Allocate a TaskStatus_t structure for each task.  An array could be
  // allocated statically at compile time.
  pxTaskStatusArray = (TaskStatus_t*) pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );

  if ( pxTaskStatusArray != NULL )  {
    // Generate raw status information about each task.
    uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalRunTime );

    // For percentage calculations.
    ulTotalRunTime /= 100UL;

    // Avoid divide by zero errors.
    if ( ulTotalRunTime > 0 )
    {
      // For each populated position in the pxTaskStatusArray array,
      // format the raw data as human readable ASCII data
      for ( x = 0; x < uxArraySize; x++ )
      {
        // What percentage of the total run time has the task used?
        // This will always be rounded down to the nearest integer.
        // ulTotalRunTimeDiv100 has already been divided by 100.
        ulStatsAsPercentage = pxTaskStatusArray[ x ].ulRunTimeCounter / ulTotalRunTime;

        if ( ulStatsAsPercentage > 0UL )
        {
          printf( "%-20s     %9u%10u%% %7d", pxTaskStatusArray[ x ].pcTaskName, pxTaskStatusArray[ x ].ulRunTimeCounter, ulStatsAsPercentage, (int)pxTaskStatusArray[ x ].usStackHighWaterMark  );
        }
        else
        {
          // If the percentage is zero here then the task has
          // consumed less than 1% of the total run time.
          printf( "%-20s     %10u        <1%% %7d", pxTaskStatusArray[ x ].pcTaskName, pxTaskStatusArray[ x ].ulRunTimeCounter, (int)pxTaskStatusArray[ x ].usStackHighWaterMark );
        }
        state = pxTaskStatusArray[ x ].eCurrentState;
        currentPriority = pxTaskStatusArray[ x ].uxCurrentPriority;
        printf("s:%d p:%d(%d)\n", state, currentPriority, currentPriority);
      }
    }

    // The array is no longer needed, free the memory it consumes.
    vPortFree( pxTaskStatusArray );
  }
}

String getFilename(String path, String ps) {

  int indexOfLastPS;
  String deslashed;

  if (path == ps || path == "") return ("");

  deslashed = slash("", path, ""); // remove last slash,
  indexOfLastPS = deslashed.lastIndexOf(ps);
  //Serial.println(path);Serial.println(deslashed);Serial.println(indexOfLastPS);
  if (indexOfLastPS == -1 ) {
    return path;
  } else {
    return deslashed.substring(indexOfLastPS + 1);
  }
}
