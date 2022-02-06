#include <Arduino.h>
#include <CAN.h>

int probeState = 1;

/**
 * @brief Setup CAN bus
 *
 */
void canSetup()
{
  Serial.println("CAN Receiver");
  // start the CAN bus at 500 kbps
  if (!CAN.begin(500E3))
  {
    Serial.println("Starting CAN failed!");
    while (1)
      ;
  }
}

/**
 * @brief Read Can Bus
 *
 */
void canReadLoop()
{

  int packetSize = CAN.parsePacket();

  if (packetSize)
  {
    // received a packet
    Serial.print("Received ");

    if (CAN.packetExtended())
    {
      Serial.print("extended ");
    }

    if (CAN.packetRtr())
    {
      // Remote transmission request, packet contains no data
      Serial.print("RTR ");
    }

    Serial.print("packet with id 0x");
    Serial.print(CAN.packetId(), HEX);

    if (CAN.packetRtr())
    {
      Serial.print(" and requested length ");
      Serial.println(CAN.packetDlc());
    }
    else
    {
      Serial.print(" and length ");
      Serial.println(packetSize);
    }
    while (CAN.available())
    {
      Serial.print(" ");
      Serial.print(CAN.read(), HEX);
    }
    Serial.println("");
  }
}

/**
 * @brief Action to take when touchprobe detects edge
 * IRAM_ATTR identifier to keep it in RAM and have faster execution
 */
void IRAM_ATTR triggerEnd()
{
  int state = digitalRead(GPIO_NUM_33);
  if (state != probeState)
  {
    probeState = state;
    // should be between IFDEBUGS or something
    Serial.print("!!!!!!!! Touch Change : ");
    Serial.print(state);
    Serial.println(" !!!!!!!!");

    // Sendin a CAN message from within the interrupt doesn't work, so just changing the state and sending the message in the main loop
    /*
        CAN.beginPacket(0x604);
        CAN.write(state);  //0 triggered, 1 when not triggered
        CAN.endPacket();
      */
  }
}

/**
 * @brief Attach interrupt to take action when touchprobe detects edge
 *
 */
void pinInterruptSetup()
{
  pinMode(GPIO_NUM_33, INPUT_PULLDOWN);
  // It is not possible to have an interrupt on both FALLING and RISING, so declare one on CHANGE and read the state in the interrupt
  attachInterrupt(digitalPinToInterrupt(GPIO_NUM_33), triggerEnd, CHANGE);
}

/**
 * @brief Set everything up
 *
 */
void setup()
{
  Serial.begin(9600);
  while (!Serial)
    ;
  canSetup();
  pinInterruptSetup();
}

int oldState = -1;
/**
 * @brief loop
 *
 */
void loop()
{
  canReadLoop();


  // id 604 and 605 have been seen for probe, value is 0 when bed detected, 1 when moving away
  if (probeState != oldState)
  {
    oldState = probeState;
    CAN.beginPacket(0x605);
    CAN.write(probeState); // 0 triggered, 1 when not triggered
    CAN.endPacket();
  }
}
