/*
 ESP8266 Blink by Simon Peter
 Blink the blue LED on the ESP-01 module
 This example code is in the public domain
 
 The blue LED on the ESP-01 module is connected to GPIO1 
 (which is also the TXD pin; so we cannot use Serial.print() at the same time)
 
 Note that this sketch uses LED_BUILTIN to find the pin with the internal LED
*/

#define MOTSTEPL 5
#define MOTSTEPR 3

#define MOTDIRL 4
#define MOTDIRR 2

#define TASTERL 6
#define TASTERR 7
#define STEPDELAY 1

#define INITHRESHOLD 800
#define SAFETYTHRESHOLD_OUT 640
#define SAFETYTHRESHOLD_IN 640

#define BREAKRAMP_LEFT_OPEN 1740
#define BREAKRAMP_LEFT_CLOSE 1800

#define BREAKRAMP_RIGHT_OPEN 1765
#define BREAKRAMP_RIGHT_CLOSE 1750

int tlState = 0;
int trState = 0;

bool blOpen = false;
bool brOpen = false;

bool isInDriveL = false;
bool isInDriveR = false;

bool breakRightActive = false;
bool breakLeftActive = false;

int motorVeloR = 0;
int motorVeloL = 0;

bool emergencyStop = false;
bool muteCurrentSensor = false;

int counter = 0;

void setup() {
  pinMode(MOTSTEPL,OUTPUT);
  pinMode(MOTSTEPR,OUTPUT);
  
  pinMode(MOTDIRL,OUTPUT);
  pinMode(MOTDIRR,OUTPUT);
  delay(1000);
   
  pinMode(TASTERL,INPUT);
  pinMode(TASTERR,INPUT);
  
  digitalWrite(TASTERL, HIGH);
  digitalWrite(TASTERR, HIGH);

  Serial.begin(9600);
  delay(1000);
  Serial.println("Begin");

  tlState = digitalRead(TASTERL);
  trState = digitalRead(TASTERR);
  
  ReferenceDrive();
}

// the loop function runs over and over again forever
void loop() {
  CheckSensor();

  int iniInLeft = analogRead(A1);
  int iniOutLeft = analogRead(A2);
  int iniInRight = analogRead(A4);
  int iniOutRight = analogRead(A3);
  
  if ((!blOpen && iniInLeft < INITHRESHOLD) || (blOpen && iniOutLeft < INITHRESHOLD))
  {
    Serial.println("BREAK LEFT");
    BreakLeft();
  }

  if ((!brOpen && iniInRight < INITHRESHOLD) || (brOpen && iniOutRight < INITHRESHOLD))
  {
    Serial.println("BREAK RIGHT");
    BreakRight();
  }

  UpdateDrive();
}

void CheckSensor()
{
  if(!muteCurrentSensor)
  {
    int safety = analogRead(A0);
    if(safety > SAFETYTHRESHOLD_OUT && (blOpen || brOpen) || safety > SAFETYTHRESHOLD_IN && (!blOpen || !brOpen))
    {
      counter++;
      if(counter > 90)
      {
        //emergencyStop = true;
        counter = 0;
      }
    }
    else
    {
      counter = 0;
    }
  }
  else
  {
    counter = 0;
  }

  if(digitalRead(TASTERR) != trState)
  {
    Serial.println("TOGGLE RIGHT");
    trState = digitalRead(TASTERR);
    if(emergencyStop)
    {
      Serial.println("RESET EMERGENCY");
      emergencyStop = false;
    }

    if(isInDriveL || isInDriveR)
    {
      Serial.println("RIGHT is In Drive --> ESTOP");
      emergencyStop = true;
    }
    else
    {
      ToggleRight();
    }
  }
  
  if(digitalRead(TASTERL) != tlState)
  {
    Serial.println("TOGGLE LEFT");
    tlState = digitalRead(TASTERL);

    if(emergencyStop)
    {
      Serial.println("RESET EMERGENCY");
      emergencyStop = false;
    }

    if(isInDriveL || isInDriveR)
    {
      Serial.println("LEFT is In Drive --> ESTOP");
      emergencyStop = true;
    }
    else
    {
      ToggleLeft();
    }
  }
}

void UpdateDrive()
{
  if(isInDriveR)
  {
    Step(MOTSTEPR);
  }
  else if (breakRightActive)
  {
    int length = 0;
    if(brOpen)
    {
      length = BREAKRAMP_RIGHT_OPEN;
    }
    else
    {
      length = BREAKRAMP_RIGHT_CLOSE;
    }
    for(int i = length; i > 0 && !emergencyStop; i--)
    {
      if(i < 50)
      {
        muteCurrentSensor = true;
      }

      SetMotorVelocity(MOTSTEPR, map(i,0,length,0,65));
      Step(MOTSTEPR);
    }
    muteCurrentSensor = false;
    breakRightActive = false;
  }

  if(isInDriveL)
  {
    Step(MOTSTEPL);
  }
  else if (breakLeftActive)
  {
    int length = 0;
    if(blOpen)
    {
      length = BREAKRAMP_LEFT_OPEN;
    }
    else
    {
      length = BREAKRAMP_LEFT_CLOSE;
    }

    for(int i = length; i > 0 && !emergencyStop; i--)
    {
      if(i < 50)
      {
        muteCurrentSensor = true;
      }

      SetMotorVelocity(MOTSTEPL, map(i, 0, length, 0, 65));
      Step(MOTSTEPL);
    }

    muteCurrentSensor = false;
    breakLeftActive = false;
  }
}

void Step(int pin)
{
  CheckSensor();

  if(emergencyStop)
  {
    SetMotorVelocity(pin, 0);
    return;
  }

  int timeout = 2000;
  if(pin == MOTSTEPL)
  {
    timeout = motorVeloL;
  }
  else if (pin == MOTSTEPR)
  {
    timeout = motorVeloR;
  }

  digitalWrite(pin, HIGH);
  delayMicroseconds(timeout);
  digitalWrite(pin, LOW);
}

void StartOpenLeft()
{
  blOpen = true;
  setDriveDircetion(MOTDIRL, HIGH);
  StartDrive(MOTSTEPL);
  SetMotorVelocity(MOTSTEPL, 100);
}

void StartCloseLeft()
{
  blOpen = false;
  setDriveDircetion(MOTDIRL, LOW);
  StartDrive(MOTSTEPL);
  SetMotorVelocity(MOTSTEPL, 100);
}

void StartOpenRight()
{
  brOpen = true;
  setDriveDircetion(MOTDIRR, HIGH);
  StartDrive(MOTSTEPR);
  SetMotorVelocity(MOTSTEPR, 100);
}

void StartCloseRight()
{ 
  brOpen = false;
  setDriveDircetion(MOTDIRR, LOW);
  StartDrive(MOTSTEPR);
  SetMotorVelocity(MOTSTEPR, 100);
}

void StartDrive(int pin)
{
  int length = 500;
  muteCurrentSensor = true;
  for(int i = 0; i < length && !emergencyStop; i++)
  {
    SetMotorVelocity(pin, map(i, 0, length, 0, 68));
    Step(pin);
  }
  muteCurrentSensor = false;
}

void BreakLeft()
{
  breakLeftActive = true;
  SetMotorVelocity(MOTSTEPL, 0);
}

void BreakRight()
{
  breakRightActive = true;
  SetMotorVelocity(MOTSTEPR, 0);
}

void Stop()
{
  SetMotorVelocity(MOTSTEPR, 0);
  SetMotorVelocity(MOTSTEPL, 0);

  breakRightActive = false;
  breakLeftActive = false;
}

void setDriveDircetion(int dirPin, int motorDirection)
{
  digitalWrite(dirPin, motorDirection);
}


void SetMotorVelocity(int pin, int percentage)
{
  if(pin == MOTSTEPL)
  {
    isInDriveL = percentage > 0;
    motorVeloL = calculateMotorVelocity(percentage);
  } 
  else if(pin == MOTSTEPR)
  {
    isInDriveR = percentage > 0;
    motorVeloR = calculateMotorVelocity(percentage);
  }
}

int calculateMotorVelocity(int percentage)
{
  return map(100 - percentage, 0, 100, 35, 2000);
}

void ReferenceDrive()
{
    StartOpenRight();
    StartOpenLeft();
} 

void ToggleLeft()
{
  if(blOpen)
  {
    StartCloseLeft();
  }
  else
  {
    StartOpenLeft();
  }
}

void ToggleRight()
{
  if(brOpen)
  {
    StartCloseRight();
  }
  else
  {
    StartOpenRight();
  }
}