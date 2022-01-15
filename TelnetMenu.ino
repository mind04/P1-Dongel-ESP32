/* 
***************************************************************************  
**  Program  : menuStuff, part of DSMRloggerAPI
**  Version  : v4.0.0
**
**  Copyright (c) 2021 Willem Aandewiel / Martijn Hendriks
**
**  TERMS OF USE: MIT License. See bottom of file.                                                            
***************************************************************************      
*/

void DisplayFile(const char *fname) { 
  if (bailout() || !FSmounted) return; //exit when heapsize is too small
  File RingFile = LittleFS.open(fname, "r"); // open for reading
  if (RingFile)  {
    DebugTln(F("Ringfile output (telnet only): "));
    //read the content and output to serial interface
    while (RingFile.available()) TelnetStream.println(RingFile.readStringUntil('\n'));
    Debugln();    
  } else DebugT(F("LogFile doesn't exist: "));
  RingFile.close();
} //displaylogfile

//--------------------------------
void P1Update(bool sketch){
  char versie[30] = "";

  //clear buffer
  while (TelnetStream.available() > 0) { (char)TelnetStream.read(); yield(); }
  
  Debugln(F("\n/!\\ UPDATE MODULE /!\\"));
  Debugf("Geef update %s versie op (bv. 3.1.1): ",sketch?"SKETCH":"FILE");
  TelnetStream.setTimeout(10000);
  TelnetStream.readBytesUntil('\n', versie, sizeof(versie)); 
  TelnetStream.setTimeout(1000);
  
  versie[strlen(versie)-1] = '\0'; //remove enter

  if (strlen(versie)>4) RemoteUpdate(versie,sketch); 
  else Debugln(F("Fout in versie opgave: formaat = x.x.x")); 
}
//--------------------------------

void ResetDataFiles() {
  LittleFS.remove("/RNGdays.json");
  LittleFS.remove("/RNGhours.json");
  LittleFS.remove("/RNGmonths.json");
  LittleFS.remove("/P1.old");
  LittleFS.remove("/P1.log");
  LittleFS.remove("/Reboot.log");      //pre 3.1.1 
  LittleFS.remove("/Reboot.old");      //pre 3.1.1  
  LittleFS.remove("/DSMRstatus.json"); //pre 3.1.1 
  DebugTln(F("Datafiles are reset"));
}

//===========================================================================================
void displayBoardInfo() 
{
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  snprintf(cMsg, sizeof(cMsg), "model %x rev: %x cores: %x", chip_info.model, chip_info.revision, chip_info.cores);

  Debugln(F("\r\n==================================================================\r"));
  Debug(F("]\r\n      Firmware Version ["));  Debug( _VERSION );
  Debug(F("]\r\n              Compiled ["));  Debug( __DATE__ "  "  __TIME__ );
  Debug(F("]\r\n              #defines "));   Debug(F(ALL_OPTIONS));
  Debug(F(" \r\n   Telegrams Processed ["));  Debug( telegramCount );
  Debug(F("]\r\n           With Errors ["));  Debug( telegramErrors );
  Debug(F("]\r\n              FreeHeap ["));  Debug( ESP.getFreeHeap() );
  Debug(F("]\r\n             max.Block ["));  Debug( ESP.getMaxAllocHeap() );
  Debug(F("]\r\n               Chip ID ["));  Debug( WIFI_getChipId() );
  Debug(F("]\r\n             Chip Info ["));  Debug( cMsg );
  Debug(F("]\r\n           SDK Version ["));  Debug( ESP.getSdkVersion() );
  Debug(F("]\r\n        CPU Freq (MHz) ["));  Debug( ESP.getCpuFreqMHz() );
  Debug(F("]\r\n      Sketch Size (kB) ["));  Debug( ESP.getSketchSize() / 1024.0 );
  Debug(F("]\r\nFree Sketch Space (kB) ["));  Debug( ESP.getFreeSketchSpace() / 1024.0 );
  Debug(F("]\r\n  Flash Chip Size (kB) ["));  Debug( ESP.getFlashChipSize() / 1024 );
  Debug(F("]\r\n          FS Size (kB) ["));  Debug( LittleFS.totalBytes() / 1024 );
  Debug(F("]\r\n      Flash Chip Speed ["));  Debug( ESP.getFlashChipSpeed() / 1000 / 1000 );
  
  FlashMode_t ideMode = ESP.getFlashChipMode();
  Debug(F("]\r\n       Flash Chip Mode ["));  Debug( flashMode[ideMode] );

  Debugln(F("]\r"));

  Debugln(F("==================================================================\r"));
  Debug(F(" \r\n            Board type ["));
#ifdef ESP32_DEV
    Debug(F("ESP32_DEV"));
#endif
#ifdef ESP32S2_DEV
    Debug(F("ESP32S2_DEV"));
#endif
  Debug(F("]\r\n                  SSID ["));  Debug( WiFi.SSID() );
#ifdef SHOW_PASSWRDS
  Debug(F("]\r\n               PSK key ["));  Debug( WiFi.psk() );
#else
  Debug(F("]\r\n               PSK key [**********"));
#endif
  Debug(F("]\r\n            IP Address ["));  Debug( WiFi.localIP().toString() );
  Debug(F("]\r\n              Hostname ["));  Debug( settingHostname );
  Debug(F("]\r\n     Last reset reason ["));  Debug( getResetReason() );
  Debug(F("]\r\n                upTime ["));  Debug( upTime() );
  Debugln(F("]\r"));

  Debugln(F("==================================================================\r"));
  Debug(F(" \r\n           MQTT broker ["));  Debug( settingMQTTbroker );
  Debug(F("]\r\n             MQTT User ["));  Debug( settingMQTTuser );
  #ifdef SHOW_PASSWRDS
    Debug(F("]\r\n         MQTT PassWord ["));  Debug( settingMQTTpasswd );
  #else
    Debug(F("]\r\n         MQTT PassWord [**********"));
  #endif
  Debug(F("]\r\n             Top Topic ["));  Debug(settingMQTTtopTopic );
  Debug(F("]\r\n       Update Interval ["));  Debug(settingMQTTinterval);
  Debugln(F("]\r"));
  Debugln(F("==================================================================\r\n\r"));

} // displayBoardInfo()

//===========================================================================================
void handleKeyInput() 
{
  char    inChar;

  while (TelnetStream.available() > 0) 
  {
    yield();
    inChar = (char)TelnetStream.read();
    
    switch(inChar) {
      case 'a':     
      case 'A':     { char c;
                      while (TelnetStream.available() > 0) { 
                        c = (char)TelnetStream.read();
                        switch(c){
                        case 'r': P1StatusRead(); break;
                        case 'w': P1StatusWrite(); break;
                        case 'p': P1StatusPrint(); break;
                        case 'x': ReadEepromBlock();break;
                        case 'z': P1StatusReset(); break;
                        default : Debugln(F("P1 Status info:\nr = read\nw = write\np = print\nz = erase"));
                        } //switch
                        while (TelnetStream.available() > 0) {(char)TelnetStream.read();} //verwijder extra input
                      } //while
                      break; }
      case 'b':
      case 'B':     displayBoardInfo();
                    break;
      case 'l':
      case 'L':     readSettings(true);
                    break;
      case 'd':
      case 'D':     { char c;
                      while (TelnetStream.available() > 0) { 
                        c = (char)TelnetStream.read();
                        switch(c){
                        case 'b': displayBoardInfo();break;
                        case 'd': DisplayFile("/RINGdays.json"); break;; break;
                        case 'h': DisplayFile("/RINGhours.json"); break;
                        case 'm': DisplayFile("/RINGmonths.json"); break;
                        case 'l': DisplayFile("/P1.log");break;
                        case 's': listFS();break;
                        default : Debugln(F("Display:\nb = board info\nd = Day table from FS\nh = Hour table from FS\nm = Month table from FS\nl = Logfile from FS\ns = File info"));
                        } //switch
                        while (TelnetStream.available() > 0) {(char)TelnetStream.read();} //verwijder extra input
                      } //while
                      break; }
      case 'E':     eraseFile();
                    break;
      #if defined(HAS_NO_SLIMMEMETER)
      case 'O':     forceBuildRingFiles = true;
                    runMode = SInit;
                    break;
      #endif
      case 'f':
      case 'F':     listFS();
                    break;                        
      case 'W':     Debugf("\r\nConnect to AP [%s] and go to ip address shown in the AP-name\r\n", settingHostname);
                    delay(1000);
                    WiFi.disconnect(true);  // deletes credentials !
                    //setupWiFi(true);
                    P1Reboot();
                    break;
      case 'p':
      case 'P':     showRaw = !showRaw;
                    break;
                    
      case 'Q':     ResetDataFiles();
                    break;                      
                    
      case 'R':     DebugFlush();
                    P1Reboot();
                    break;
                    
      case 'S':     P1Update(false);
                    break;
                    
      case 'U':     P1Update(true);
                    break;
                    
      case 'v':
      case 'V':     if (Verbose2) 
                    {
                      Debugln(F("Verbose is OFF\r"));
                      Verbose1 = false;
                      Verbose2 = false;
                    } 
                    else if (Verbose1) 
                    {
                      Debugln(F("Verbose Level 2 is ON\r"));
                      Verbose2 = true;
                    } 
                    else 
                    {
                      Debugln(F("Verbose Level 1 is ON\r"));
                      Verbose1 = true;
                      Verbose2 = false;
                    }
                    break;                    
#ifdef USE_WATER_SENSOR
      case 'x':
      case 'X':     DebugTf("Watermeter readings: %i m3 and %i liters\n",P1Status.wtr_m3,P1Status.wtr_l);
                    break;
#endif
      case 'Z':     P1Status.sloterrors = 0;
                    P1Status.reboots    = 0;
                    P1Status.wtr_m3     = 0;
                    P1Status.wtr_l      = 0;
                    telegramCount       = 0;
                    telegramErrors      = 0;
                    P1StatusWrite();
                    break;
                    
      default:      Debugln(F("\r\nCommands are:\r\n"));
                    Debugln(F("   A  - P1 Status info a=available|r=read|w=write|p=print|z=erase\r"));
//                    Debugln(F("   B  - Board Info\r"));
                    Debugln(F("  *E  - erase file from FS\r"));
                    Debugln(F("   F - File info on SPIFFS\r"));                    
                    Debugln(F("   L  - list Settings\r"));
                    Debugln(F("   D+ - Display b=board info | d=Day table | h=Hour table | m=Month table | l=Logfile | s = File info\r"));
//                    Debugln(F("   H  - Display Hour table from FS\r"));
//                    Debugln(F("   N  - Display LogFile P1.log\r"));
//                    Debugln(F("   M  - Display Month table from FS\r"));
                    #ifdef HAS_NO_SLIMMEMETER
                      Debugln(F("  *O  - Force build RING files\r"));
                    #endif
                    Debugln(F("   P  - No Parsing (show RAW data from Smart Meter)\r"));
                    Debugln(F("  *W  - Force Re-Config WiFi\r"));
                    Debugln(F("  *R  - Reboot\r"));
                    Debugln(F("  *S+ - Update File System: Enter version -> S4.0.1\r"));
                    Debugln(F("  *U+ - Update Remote; Enter Firmware version -> U 4.0.1 \r"));
#ifdef USE_WATER_SENSOR
                    Debugln(F("   X  - Watermeter reading\r"));
#endif                    
                    Debugln(F("  *Z  - Zero counters\r\n"));
                    if (Verbose1 & Verbose2)  Debugln(F("   V  - Toggle Verbose Off\r"));
                    else if (Verbose1)        Debugln(F("   V  - Toggle Verbose 2\r"));
                    else                      Debugln(F("   V  - Toggle Verbose 1\r"));


    } // switch()
    while (TelnetStream.available() > 0) 
    {
       yield();
       (char)TelnetStream.read();
    }
  }
  
} // handleKeyInput()


/***************************************************************************
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to permit
* persons to whom the Software is furnished to do so, subject to the
* following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
* OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
* THE USE OR OTHER DEALINGS IN THE SOFTWARE.
* 
***************************************************************************/
