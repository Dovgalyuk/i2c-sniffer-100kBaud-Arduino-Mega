#pragma GCC optimize ("-O3")

//////////////////////////
// i2c sniffer
//////////////////////////

// For Arduino Due (lots of RAM and MHz)
// Uses direct port access for maximum speed
// Do not change pin assignments unless you know what you are doing!
// They are hardcoded in SAMPLE, gbuffer(), START1 and START2 for maximum speed

// The data is displayed in the Serial Monitor, set to 115200 baud

#define SDA_MASK          0x2                        // Pin 26 (PD1) on PIND is used for the SDA line
#define SCL_MASK          0x1                        // Pin 25 (PD0) on PIND is used for the SCL line
#define START1            0x3                        // start condition: transition from START1 to START2
#define START2            0x1

#define BUFFSIZE          64000

// The Aduino is not fast enough to acquire and display the data at the same time.
// This is usually not a problem because the i2c access normally happens in bursts.
// Set TIMEOUT below longer than the maximum length of one burst, but much shorter than
// the time beween bursts minus the time required to analye and display the data.
//
// Example: maximum time of one i2c burst  300 msec
// Time between bursts: 3000 msec
// Time to analyze and display data of one burst: 100 msec
// >>> TIMEOUT can be set between 300 msec and 2900 msec
//
// If you don't know anything about the i2c activity, set the timeout initially to a large value, for example 10000 msec
// This means that the sampling will go on for 10000 msec after a first i2c start condition has been observed.

#define TIMEOUT           1000                         // timeout in msec, change as required

byte buffer[BUFFSIZE];

static volatile uint32_t *dataport;
#define SAMPLE ((*dataport) & 0x3)

////////////
void setup()
////////////
{
  // portD
  pinMode(25, INPUT);
  pinMode(26, INPUT);
  dataport = portInputRegister(digitalPinToPort(25));

  Serial.begin(115200);
  Serial.println("\ni2c sniffer by rricharz\n");
}

////////////////////////
void printHexByte(int b)
////////////////////////
// Startdard HEX printing does not print leading 0
{
  // Serial.print("0x");
  Serial.print((b >> 4) & 0xF, HEX);
  Serial.print(b & 0xF, HEX);
}

//////////////////
int acquire_data()
//////////////////
//
// Pins are hard coded for speed reason
{
  unsigned long endtime;
  uint8_t data,lastData;

  Serial.println("Acquiring data");

  // wait until start condition is fullfilled
  /*bool start = false;
  while (!start) {
    while ((lastData = SAMPLE) != START1);          // wait until state is START1
    while ((data = SAMPLE) == lastData);            // wait until state change
    start = (data == START2);                       // start condtion met if new state is START2
  }*/

  //endtime = millis() + TIMEOUT;
  lastData = START2;
  int k = 0;
  //buffer[k++] = START1;
  //buffer[k++] = START2;
  do {
    
    while ((data = SAMPLE) == lastData);           // wait until data has changed
    buffer[k++] = lastData = data;
  }
  while ((k < BUFFSIZE)/* && (millis() < endtime)*/);
  return k;
}

//////////////////
int gbuffer(int i)
//////////////////
{
    return buffer[i] & 0x3;
}

/////////////////////////////////
int findNextStartCondition(int k)
/////////////////////////////////
{
  while((k < BUFFSIZE- 1) && ((gbuffer(k - 1) != 3) || (gbuffer(k) != 1)))
    k++;
    // Serial.print("Next start condition: "); Serial.println(k);
  return k;
}

/////////////////////////////
void display_data(int points)
/////////////////////////////
{
  int lastData, data, k, Bit, Byte, i, state,nextStart;
  //long starttime;

#define ADDRESS  0         // First state, address follows

  //starttime = millis();

/*
  // display raw data
  Serial.print("Raw transitions, number of transitions = ");
  Serial.println(points);
  Serial.print("Each number represents a status, bit 1 = SDA, bit 0 = SCL, start condition = 31");
  for (k = 0; k < points; k++) {
    if ((gbuffer(k) == 3) && (gbuffer(k + 1) == 1))        // start condition
      Serial.println();
    Serial.print(gbuffer(k));
  }
*/

  //Serial.print("Analyzing data, number of transitions = "); Serial.println(points);
  //Serial.println("i2c bus activity: * means ACQ = 1 (not ok)");
  k = findNextStartCondition(1) + 1;              // ignore start transition
  i = 0;
  Byte = 0;
  state = ADDRESS;
  nextStart = findNextStartCondition(k);
  do {
    data = gbuffer(k++);
    if (data & 1) {
      Bit = (data & 2) > 0;
      Byte = (Byte << 1) + Bit;
      if (i++ >= 8) {
        if (state == ADDRESS) {
          //Serial.print("Dev=");
          printHexByte(Byte / 4);
          if (Byte & 2)
            Serial.print(" R");
          else
            Serial.print(" W");
          /*if (Byte & 1)
            Serial.print("*");
          else*/
            Serial.print(" ");
          state++;
        }       
        /*else if (state == 1) {
          Serial.print("Data=");
          printHexByte(Byte / 2);
          if (Byte & 1)
            Serial.print("*");
          else
            Serial.print(" ");
          state++;
        }*/
        else {
          printHexByte(Byte / 2);
          /*if (Byte & 1)
            Serial.print("*");
          else*/
            Serial.print(" ");
        }
        if (nextStart - k < 9) {
          Serial.println();
          k = nextStart + 1;
          nextStart = findNextStartCondition(k);
          state = ADDRESS;
        }
        i = 0;
        Byte = 0;
      }
    }
  }
  while (k < points);
  //Serial.print("Time to analyze and display data was ");
  //Serial.print(millis() - starttime);
  //Serial.println(" msec");
  Serial.println();
}

///////////
void loop()
///////////
{
  int points = acquire_data();
  display_data(points);
}
