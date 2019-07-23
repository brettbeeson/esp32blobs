#pragma once

#include <Arduino.h>
#include <vector>

//namespace bblib {
void vTaskPrintRunTimeStats();
void taskState(TaskHandle_t task);
const String ps = "/";
String slash(const String& stripFromStart, const String&from , const String& stripFromEnd);
String slash(char withStart, const String&from);
String slash(const String&from, char withEnd);
std::vector <String> ls(const String& localDir, int maxFiles, int* nTotalFiles, const String& endingWith = "", const String& localFilePrefix = "/sdcard/");
unsigned long mround_ul(unsigned long  n, int m);
String resetReason(int core=0);
String getFilename(String path,String ps="/");

class LogFileClass {
  public:
    LogFileClass(const char* filename);
    //void printf(const char* message, ...);
    void print(const String& message);
    //void print(const char* message);
    void read();
    String tail(int lines=10);
    String head(int lines=10);
    String filename;
};
// Define in main
extern LogFileClass LogFile;


//}
