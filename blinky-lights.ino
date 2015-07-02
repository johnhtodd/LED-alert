/* "LED Alert" with web server API                          */
/* (c)2014 John Todd  jtodd@loligo.com                      */
/*                                                          */
/* github: johnhtodd/LED-Alert                              */

/* This uses the Total Control Lighting LED Pixels          */
/* and control shield.  See http://www.coolneon.com/        */
/* for hardware and https://github.com/CoolNeon/arduino-tcl */
/* for the TCL Arduino drivers by Christopher De Vries      */

/* This API is super-robust, because it never barfs on      */
/* errors. In fact, it always gives the same happy result.  */
/* Not enough room in Aurdino for good error checking so... */

/* You'll need to change the MAC address (maybe) and the IP */
/* address (certainly) contained in the file below. Also    */
/* change the ledBank[BANK_TOTAL] statement if you want to  */
/* have more than two banks of a dozen or so LEDs.          */


/* Examples:                                                                            */
/*  bank1 to solid red:                                                                 */
/*   http://1.2.3.4/?bank1-1=red                                                        */
/*  Blink bank1 between red and green:                                                  */
/*   http://1.2.3.4/?bank1-1=red&delay1-1=1000&bank1-2=green&delay1-2=1000              */
/* Rapid flash bank1 red and bank2 solid green                                          */
/*   http://1.2.3.4/?bank1-1=red&delay1-1=200&bank1-2=black&delay1-2=200&bank2-1=green  */



#include <SPI.h>
#include <TCL.h>
#include <Ethernet.h>

/* Comment following line of code if serial debug is not required */
#define DEBUG 

/*
 Memory balance - estimate between BANK_TOTAL and MAX_SEQUENCE_TOTAL
   BANK_TOTAL = 1360/(8+5*MAX_SEQUENCE_TOTAL)
 This keeps RAM approx 5% free so it should work. If you get an error message
 while uploading, you should reduce the BANK_TOTAL number of MAX_SEQUENCE_TOTAL
 */

const byte BANK_TOTAL=15;
const byte MAX_SEQUENCE_TOTAL=10; //maximum number of sequences among the banks

/* array to contain number of leds in a bank
Example:
  static byte ledBank[BANK_TOTAL]={12,11,2}
This means that bank1 has 12 LEDs, starting at LED1. bank2 has 11 LEDs, starting at 
LED13 and bank2 has 2 LEDs starting at LED24
*/
static byte ledBank[BANK_TOTAL]={12,13}; //should be initialized or setting up number of LEDs in a Bank


/*
Array to contain number of sequences in a bank
*/
static byte sequenceTotal[BANK_TOTAL];

/*
Arrary to contain last time 
This is used to keep track of time to throw color data to TCL
*/
static unsigned long lastTime[BANK_TOTAL]; //may be int if delay/duration is up to 65 sec.

/*
Array to contain current sequence index: this is used to select color in a bank
*/
static byte sequence_index[BANK_TOTAL];

struct Color_Segment //contains color and delay
{
  byte red;
  byte green;
  byte blue;
  unsigned int duration;
};

static Color_Segment bank[BANK_TOTAL][MAX_SEQUENCE_TOTAL];



byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 1, 222);

EthernetServer server(80);

/* this reserves 255 byte for GETBuffer */
static String GETBuffer="xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"; //if this size is doubled then it won't work because all loop counters are single byte
void update_pattern() //get http and parse it and update the pattern
{
  byte startIndex=0;
  byte endIndex=0;
  byte bankIndex=0;
  byte sequenceIndex=0;
  byte red=0;
  byte green=0;
  byte blue=0;
  byte fstLoop[BANK_TOTAL];
  
  String temp="xxxxxxxxxx";
  
  /* making case insensitive */
  GETBuffer.toUpperCase();
  /* adding '&' at the end of Buffer: simplifying parsing */
  startIndex=GETBuffer.indexOf(F("?")); //better get index of HTTP... will implement in next version.
  startIndex=GETBuffer.indexOf(F(" "),startIndex);
  GETBuffer[startIndex]='&';

  startIndex=0;
  
  for(byte i=0;i<BANK_TOTAL;i++)
  {
    fstLoop[i]=0;
  }
  
#ifdef DEBUG
  Serial.println(GETBuffer);
#endif
  //taking care of bank
  while(1)
  {
    
    startIndex=GETBuffer.indexOf(F("BANK"),startIndex);
    if(startIndex<GETBuffer.length()) //don't exceed buffer size more than 254 bytes
    {
      endIndex=GETBuffer.indexOf(F("-"),startIndex);
      temp = GETBuffer.substring(startIndex+4,endIndex); //bank number
      bankIndex= (byte)temp.toInt()-1;
      startIndex=endIndex;
      endIndex=GETBuffer.indexOf(F("="),startIndex);
      temp=GETBuffer.substring(startIndex+1,endIndex); //sequence number
      sequenceIndex=(byte)temp.toInt()-1;
      //resets bank sequence
      //sequences must start from 1 and goes to n (1,2,3,.........MAX_SEQUENCE_TOTAL) i.e. continuous from 1 to MAX
      if(fstLoop[bankIndex]==0)
      {
        sequenceTotal[bankIndex]=0;
        fstLoop[bankIndex]++;
      }
      if((sequenceIndex+1)>sequenceTotal[bankIndex])
      {
        sequenceTotal[bankIndex]=sequenceIndex+1;
      }
      startIndex=endIndex;
      endIndex=GETBuffer.indexOf(F("&"),startIndex);
      temp=GETBuffer.substring(startIndex+1,endIndex); //bank color
      
      if(temp.equals(F("BLACK")))
      {
        red=0;
        green=0;
        blue=0;
      }
      else if(temp.equals(F("WHITE")))
      {
        red=255;
        green=255;
        blue=255;
      }
      
      else if(temp.equals(F("RED")))
      {
        red=255;
        green=0;
        blue=0;
      }
      else if(temp.equals(F("GREEN")))
      {
        red=0;
        green=255;
        blue=0;
      }
      else if(temp.equals(F("BLUE")))
      {
        red=0;
        green=0;
        blue=255;
      }
      else if(temp.equals(F("YELLOW")))
      {
        red=255;
        green=255;
        blue=0;
      }
      else if(temp.equals(F("CYAN")))
      {
        red=0;
        green=255;
        blue=255;
      }
      else if(temp.equals(F("MAGENTA")))
      {
        red=255;
        green=0;
        blue=255;
      }

      
      bank[bankIndex][sequenceIndex].red=red;
      bank[bankIndex][sequenceIndex].green=green;
      bank[bankIndex][sequenceIndex].blue=blue;
    }
    else
    {
      break;
    }
    
  }
  
  //taking care of delay
  startIndex=0;
  while(1)
  {
    startIndex=GETBuffer.indexOf(F("DELAY"),startIndex);
    if(startIndex<GETBuffer.length()) //don't exceed buffer size more than 254 bytes
    {
      endIndex=GETBuffer.indexOf(F("-"),startIndex);
      temp = GETBuffer.substring(startIndex+5,endIndex); //bank number
      bankIndex= (byte)temp.toInt()-1;
      startIndex=endIndex;
      endIndex=GETBuffer.indexOf(F("="),startIndex);
      temp=GETBuffer.substring(startIndex+1,endIndex); //sequence number
      sequenceIndex=(byte)temp.toInt()-1;
      
      startIndex=endIndex;
      endIndex=GETBuffer.indexOf(F("&"),startIndex);
      temp=GETBuffer.substring(startIndex+1,endIndex); //bank color
      
      bank[bankIndex][sequenceIndex].duration=temp.toInt();
    }
    else
    {
      break;
    }
    
  }
}




void update_strand()
{
  TCL.sendEmptyFrame();
  for(byte i=0;i<BANK_TOTAL;i++)
  {
    for(byte j=0;j<ledBank[i];j++)
    {
      TCL.sendColor(bank[i][sequence_index[i]].red,bank[i][sequence_index[i]].green,bank[i][sequence_index[i]].blue);
    }
  }
  TCL.sendEmptyFrame();
}

void setup() {
  
  TCL.begin();
#ifdef DEBUG
  Serial.begin(9600);
#endif
  
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print(F("server is at "));
  Serial.println(Ethernet.localIP());
}


void loop() {
  
  // Create a client connection
  EthernetClient client = server.available();
  GETBuffer =""; //clear buffer 
  if(client)
  {
    while(client.connected())
    {
      char c=client.read();
      
      if(GETBuffer.length()<255)
      {
        GETBuffer += c;
      }
      
      //if HTTP request has ended
      if(c=='\n')
      
      /* This is probably more verbose than it needs to be, but I appreciate self-documenting services */
      {
        client.println(F("HTTP/1.1 200 OK"));
        client.println(F("Content-Type: text/html"));
        client.println(F("Connection: close"));  // the connection will be closed after completion of the response
        client.println();
        client.println(F("<!DOCTYPE HTML>"));
        client.println(F("<html>"));
        client.println(F("You sent a command - valid or invalid - no error checking here, folks!</br></br>"));
        client.println(F(" Format: http://x.y.z.a/?bankX-Y=&lt;color&gt;&delayX-Y=&lt;delay-in-ms&gt;&...  </br>"));
        client.println(F(" X increments from 1, indicating which bank (bank LED configurations are hardcoded)</br>"));
        client.println(F(" Y increments from 1, indicating which sequence of the bank is referenced</br>"));
        client.println(F(" &lt;color&gt; is one of: black, white, red, green, blue, cyan, magenta, yellow</br>"));
        client.println(F(" Limit of 254 characters in the string.</br>"));
        client.println(F(" Single banks can be specified in a single call without effecting other banks, but all</br>"));
        client.println(F(" sequences for that bank starting from 1 must be specified on a single call.</br>"));
        client.println(F(" Maximum of 10 sequences per bank.</br>"));
        client.println(F(" Maximum of 65000ms per sequence.</br>"));
        client.println(F(" Not specifying a delay means 1000ms.</br></br>"));
        client.println(F(" (c)2014 John Todd jtodd@loligo.com<br>"));
        client.println(F("</html>"));
        client.stop();
        update_pattern();
        print_pattern();
      }
    }
  }
  
  for(byte i=0;i<BANK_TOTAL;i++)
  {
    /*
    millis() rolls over after approximately 50 days!
    */
    if((millis()-lastTime[i])>bank[i][sequence_index[i]].duration)
    {
      sequence_index[i]++;
      lastTime[i]=millis();
      if(sequence_index[i]>=sequenceTotal[i])
      {
        sequence_index[i]=0;
      }
      
      update_strand();
    }
  }
  
}


void print_pattern()
{
  for(byte i=0;i<BANK_TOTAL;i++)
  {
    for(byte j=0;j<sequenceTotal[i];j++)
    {
      /*
      if delay is not defined then make it 1 sec.
      */
      if(bank[i][j].duration<1)
      {
        bank[i][j].duration=1000;
      }
#ifdef DEBUG
      Serial.print(F("Bank: "));
      Serial.print(i+1);
      
      Serial.print(F(" SEQ: "));
      Serial.print(j+1);
      Serial.print(F(" DLY: "));
      Serial.print(bank[i][j].duration);
      
      Serial.print(F("  ("));
      Serial.print(bank[i][j].red);
      Serial.print(F(","));
      Serial.print(bank[i][j].green);
      Serial.print(F(","));
      Serial.print(bank[i][j].blue);
      Serial.println(F(")"));
#endif
    }
  }
}
