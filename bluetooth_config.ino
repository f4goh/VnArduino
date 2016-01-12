// Basic Bluetooth sketch HC-06
// Communicate with a HC-06 using the serial monitor
//
// The HC-06 defaults to communication mode when first powered on 
// The default baud rate for AT mode is 9600

// See http://www.puntoflotante.net/BOLT-SYSTEM-BLUETOOTH-AT-COMMANDS.htm for details
// http://www.martyncurrey.com/arduino-and-hc-06-zs-040/
 
 
#include <SoftwareSerial.h>
SoftwareSerial BTserial(2, 3); // RX | TX
// Connect the HC-06 TX to Arduino pin 2 RX. 
// Connect the HC-06 RX to Arduino pin 3 TX through a voltage divider.
// 
 
char c = ' ';
 
void setup() 
{
    Serial.begin(9600);
    Serial.println("Arduino is ready");
    Serial.println("Remember to unselect NL & CR in the serial monitor (no end line no return)");
 
    // HC-06 default serial speed for AT mode is 9600
    BTserial.begin(9600);  
}
 
void loop()
{
 
    // Keep reading from HC-05 and send to Arduino Serial Monitor
    if (BTserial.available())
    {  
        c = BTserial.read();
        Serial.write(c);
    }
 
    // Keep reading from Arduino Serial Monitor and send to HC-05
    if (Serial.available())
    {
        c =  Serial.read();
        BTserial.write(c);  
    }
 
}


/*
 * Beware : NO CRLF !!!!!
 * AT  OK  
AT+VERSION  OKLinvorV1.8  
AT+NAMEBlueBolt   OKsetname 
AT+BAUD4       
AT+PIN1234   


    Ping Test

        No action is taken by the Bluetooth, it simply acknowledges with “OK” letting you know communication was successful.

        Send: AT

        Response: OK

    Set Baud Rate

        Sets the Bluetooth UART baud rate. Baud rate is set by an hexadecimal index from '1' to 'C'.

        Indexes are: 1:1200, 2:2400, 3:4800, 4:9600, 5:19200, 6:38400, 7:57600, 8:115200, 9:230400, A:460800, B:921600, C:1382400

        Send: AT+BAUD<index>

        Response: OK<baud rate>

    Set Bluetooth Device Name

        Sets Bluetooth Device Name

        Send: AT+NAME<device name>

        Response: OKsetname

    Set Bluetooth PIN Code

        Sets the security code needed to connect to the device.

        Send: AT+PIN<4 digit code>

        Response: OK<4 digit code>

    Check Firmware Revision

        Get The Firmware Revision Number.

        Send: AT+VERSION

        Response: Linvor V1.8

Examples:

AT
OK

AT+VERSION
OKLinvorV1.8

AT+BAUD4
OK9600

AT+PIN1234
OK1234

AT+NAMEBlueBolt
OKsetname 
 
 */
