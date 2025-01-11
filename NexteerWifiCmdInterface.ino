#include "NexteerWifiCmdInterface.h"

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

/*******************************************************************************
* Includes                                                                     *
*******************************************************************************/

/*******************************************************************************
* Classes: Public Methods                                                      *
*******************************************************************************/

/*******************************************************************************
* Name:     Setup
* Inputs:   None
* Outputs:  None
* Notes:    Sets default values for the class and configures the software 
*           serial interface.
*******************************************************************************/
void NexteerWifiCmdInterface::Setup( void )
{
    this->_espSerial.begin( NXTRWIFICMDINT_SERIAL_BAUD_RATE );
    this->WifiScanActive = false;
    return; 
}

/*******************************************************************************
* Name:     SendCommand
* Inputs:   const char* command - Command string to send to the Wi-Fi module. 
*           String response - The response instance that the function will store
*           the ESP-01 response to. 
* Outputs:  None
* Notes:    This is the overloaded form where it accepts a const char instead 
*           of a String. 
*******************************************************************************/
void NexteerWifiCmdInterface::SendCommand( const char* command, String& response )
{
    String commandString = command;
    this->SendCommand( commandString, response );
    return;
}

/*******************************************************************************
* Name:     SendCommand
* Inputs:   String command - Command string to send to the Wi-Fi module. 
*           String response - The response instance that the function will store
*           the ESP-01 response to. 
* Outputs:  None
* Notes:    This function sends the command over the software serial UART and 
*           then waits in the CheckResponse function for a properly formatted
*           response from the server. If the check response returns a retry 
*           flag, then the command will hold in a loop.  
*******************************************************************************/
void NexteerWifiCmdInterface::SendCommand( String& command, String& response )
{
    bool retryFlag = false; 

    do
    {
        /* Send command over the UART */
        this->println( command.c_str() );
        LOGDEBUG( command.c_str() );

        delay(100); /* Delay to ensure enough time is given for the ESP-01 to respond */

        response = this->CheckResponse( &retryFlag );
        response.trim();

        /* Notify of retry if debugging is turned on */
        #if ( NXTRWIFICMDINT_DEBUG_LEVEL  == 1 )
            if( retryFlag == true ) { LOGDEBUG( F("Retry SendCommand...") ); }
        #endif
    } while( retryFlag == true );

    return;
}

/*******************************************************************************
* Name:     NetworkScan
* Inputs:   int timeout - The timeout for the scan to wait for a response in 
*           milliseconds. (Optional) 
* Outputs:  None
* Notes:    Timeout defaults to 5000ms or 5 seconds. 
*******************************************************************************/
void NexteerWifiCmdInterface::NetworkScan( int timeout=5000 )
{
    /* Send Command */
    this->SendCommandAndHold( "NXTR20", timeout );
    return;
}

/*******************************************************************************
* Name:     Connect
* Inputs:   const char command - The command string to connect to the Wi-Fi. 
*           int timeout - The timeout for the connect command to wait for a 
*           connection to be established. 
* Outputs:  None
* Notes:    Timeout defaults to 10000ms or 10 seconds. This is the overloaded
*           form of this function accepting a const char command. 
*******************************************************************************/
void NexteerWifiCmdInterface::Connect( const char* command, int timeout=10000 )
{
    String commandString = command;
    this->Connect( commandString, timeout );
    return;
}

/*******************************************************************************
* Name:     Connect
* Inputs:   String - The command string to connect to the Wi-Fi. 
*           int timeout - The timeout for the connect command to wait for a 
*           connection to be established. 
* Outputs:  None
* Notes:    Timeout defaults to 10000ms or 10 seconds.
*******************************************************************************/
void NexteerWifiCmdInterface::Connect( String& command, int timeout=10000 )
{
    /* Send command */
    this->SendCommandAndHold( command, timeout );
    return;
}

/*******************************************************************************
* Classes: Private Methods                                                     *
*******************************************************************************/

/*******************************************************************************
* Name:     CheckResponse
* Inputs:   None
* Outputs:  String commandResponse - The response received from the web query
*           made by the Wi-Fi module to the server. 
* Notes:    This function has a condition that will enter an infinite loop if 
*           there is no response from the the server, or the response does not 
*           contain the proper return format. This is intentional as a way to 
*           notify you that there is a problem with the request. Issues that 
*           the Wi-Fi module can detect are reported back as an error code. 
*******************************************************************************/
String NexteerWifiCmdInterface::CheckResponse( bool* retryFlag, unsigned int timeout=5000 )
{
    String commandResponse;
    unsigned int startTime = millis();
    unsigned int currentTime = startTime;
    bool endFlag = false;

    commandResponse.reserve(50);
    *retryFlag = false;

    while( ( ( int(this->_espSerial.available()) > 0 ) || ( endFlag == false ) ) &&
           ( ( (currentTime - startTime) <= timeout) ) )
    {
        char character = this->_espSerial.read();
        if( character == '\0' ) { continue; }
        if( ( character != '\r' ) && ( character != '\n' ) && ( character < 32 ) ) { continue; }

        commandResponse += character;
        
        if ( character == '\n' )
        {
            endFlag = this->CheckForResponseCode( commandResponse );
            if ( endFlag != true )
            {
                commandResponse = "";
            }
            #if ( NXTRWIFICMDINT_DEBUG_LEVEL == 1 )
            else
            {
                LOGDEBUG( commandResponse );
            }
            #endif
        }
        currentTime = millis();
    }

    if ( ( (currentTime - startTime) >= timeout ) && 
         ( endFlag == false ) )
    {
        *retryFlag = true;
    }

    return commandResponse;
}

/*******************************************************************************
* Name:     CheckForResponseCode
* Inputs:   String data - response code from the server. 
* Outputs:  bool response - Returns true if the string contains a response code
* Notes:    Checks the string of data if the first characters are in a response
*           format, such as 0:xxxxxx or 10:xxxxxxx. 
*******************************************************************************/
bool NexteerWifiCmdInterface::CheckForResponseCode( String& data )
{
    bool response = false; 

    /* First character must be a digit 0-9 */
    if ( isDigit( data.charAt(0) ) == true )
    {
        /* Next character is either a ':' or is a digit and ':' in the 3rd position */
        if ( ( data.charAt(1) == ':' ) || 
             ( ( isDigit( data.charAt(1) ) == true ) && ( data.charAt(2) == ':' ) ) )
        {
            response = true;
        }
    }

    return response;
}

/*******************************************************************************
* Name:     write
* Inputs:   char character - Binary data. 
* Outputs:  None
* Notes:    Writes binary data to the serial port. This data is sent as a byte 
*           or series of bytes; to send the characters representing the digits 
*           of a number use the print function instead. 
*******************************************************************************/
void NexteerWifiCmdInterface::write( char character )
{
    this->_espSerial.write( character );
}

/*******************************************************************************
* Name:     println
* Inputs:   char character - value to print. 
* Outputs:  None
* Notes:    Prints data to the serial port as human-readable ASCII text 
*           followed by a carriage return character (ASCII 13, or '\r') and 
*           a newline character (ASCII 10, or '\n').
*******************************************************************************/
void NexteerWifiCmdInterface::println( char* buffer )
{
    this->_espSerial.println( buffer );
}

/*******************************************************************************
* Name:     SendCommandAndHold
* Inputs:   const char* command - Command string to send to the Wi-Fi module. 
*           unsigned int timeout - Timeout for the command to hold for a 
*           response. 
* Outputs:  None
* Notes:    This is the overloaded form where it accepts a const char instead 
*           of a String. 
*******************************************************************************/
void NexteerWifiCmdInterface::SendCommandAndHold( const char* command, unsigned int timeout )
{
    String commandString = command; 
    this->SendCommandAndHold( commandString, timeout );
    return;
}

/*******************************************************************************
* Name:     SendCommandAndHold
* Inputs:   String command - Command string to send to the Wi-Fi module. 
*           unsigned int timeout - Timeout for the command to hold for a 
*           response. 
* Outputs:  None
* Notes:    This function sends the request command to the ESP-01 over the 
*           software serial interface. The function will then hold in a while
*           loop until a response is received or a timeout has elapsed. 
*******************************************************************************/
void NexteerWifiCmdInterface::SendCommandAndHold( String& command, unsigned int timeout )
{
    unsigned int startTime = millis();
    unsigned int currentTime = startTime;

    /* Send command */
    this->println( command.c_str() );
    delay(100);

    /* Hold until timeout occurs */
    while( ( int( this->_espSerial.available() ) > 0 ) ||
           ( ( currentTime - startTime ) <= timeout ) )
    {
        if ( int( this->_espSerial.available() ) > 0 )
        {
            char character = this->_espSerial.read();
            if( character == '\0' )
                continue;
            if( ( character != '\r' ) && ( character != '\n' ) && ( character < 32 ) )
                continue;

            /* Prints to main serial monitor interface */
            Serial.print( character ); 
        }
        currentTime = millis(); /* Update the current time */
    }

    #if ( NXTRWIFICMDINT_DEBUG_LEVEL == 1 )
    if ( (currentTime - startTime) > timeout ) { LOGDEBUG(F("SendCommandAndHold Timeout Reached")); }
    #endif

    return;
}

/* EOF */