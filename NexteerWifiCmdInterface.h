/*******************************************************************************
*
* Project:  Nexteer Scholarship Competition 2024
*           Project S.T.A.R.M.A.P
* Filename: NexteerWifiCmdInterface.h
* Target:   Arduino UNO R3
* Notes:    Class definition for the Nexteer Wi-Fi Command Interface to 
*           communicate with the ESP-01 module. 
* 
*******************************************************************************/

#ifndef __NEXTEER_AT_CMD_INTERFACE_H
#define __NEXTEER_AT_CMD_INTERFACE_H

/*******************************************************************************
* Includes                                                                     *
*******************************************************************************/
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Stream.h>
#include <String.h>

/*******************************************************************************
* Macros                                                                       *
*******************************************************************************/
#define NXTRWIFICMDINT_VERSION              "1.0.0"
#define NXTRWIFICMDINT_SERIAL_BAUD_RATE     9600 /* Do not modify */

/* Debug Level 
 * Use for debugging your communication with the ESP-01 module. Note: This will 
 * display extra lines in your serial monitor. 
 * 
 * 0 - Debug Off 
 * 1 - Debug On
 */
#define NXTRWIFICMDINT_DEBUG_LEVEL  1
#define LOGDEBUG(x)  if( NXTRWIFICMDINT_DEBUG_LEVEL == 1 ) { Serial.print("[DEBUG] "); Serial.println(x); }

/*******************************************************************************
* Classes/Typedefs                                                             *
*******************************************************************************/

class NexteerWifiCmdInterface
{
    public:
        /* Data members */
        uint8_t WifiScanActive;

        /* Constructor */
        NexteerWifiCmdInterface( uint8_t RxPin, uint8_t TxPin ) : _espSerial( RxPin, TxPin )
        {
            return;
        };

        /* Methods */
        void Setup( void );
        void SendCommand( const char* command, String& response );
        void SendCommand( String& command, String& response );
        void NetworkScan( int timeout=5000 );
        void Connect( const char* command, int timeout=10000 );
        void Connect( String& command, int timeout=10000 );

    private:
        /* Data members */
        SoftwareSerial _espSerial;
        
        /* Methods */
        String CheckResponse( bool* retryFlag, unsigned int timeout=5000 );
        bool CheckForResponseCode( String& data );
        void write( char character );
        void println( char* buffer );
        void SendCommandAndHold( const char* command, unsigned int timeout );
        void SendCommandAndHold( String& command, unsigned int timeout );
};

#endif /* __NEXTEER_AT_CMD_INTERFACE_H */
