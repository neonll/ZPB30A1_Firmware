# Serial protocol

## Pins
Left side of the base PCB:
* G: GND
* R: RXD
* T: TXD
* Vc: +12V
* Others: Unused

## Settings
115200 Baud, 8N1

## Value readback
The device continously outputs it current state. Each line contains the following fields:
* Device state: 
    * ' ' (Space): Not active
    * 'A': Active and in regulation
    * 'U': Out of regulation, i.e. source can't supply enough power. As this load does not measure the current the reported current will be wrong when the load is out of regulation!
* Error (no column name): Single digit integer (see load.h for error codes)
* T: Temperature in 0.1°C
* Vi: Power supply voltage (12V nom.) in mV
* Vl: Load voltage (screw terminals) in mV
* Vs: Sense voltage (plugable connector) in mV
* I: Current in mA. As this load does not measure the current the setpoint is reported.
* mWs: Energy since start of measurement (in mWs)
* mAs: Energy since start of measurement (in mAs)

## Configuration
Configuration protocol currently is quite simple. There are two command formats:
* character\r\n: Execute a command without parameters
* characterINTEGER\r\n: Set a parameter. Integer value must fit in 16 bits

The command is executed when the \n is received. The \r is optional.

Commands: 
* !: Reset UART state. Must be sent after establishing a connection or after sending a wrong command
* R: Run
* S: Stop
* M: Mode (0=CC, 1=CW, 2=CR, 3=CV, see settings.h)
* c: Setpoint CC in mA
* w: Setpoint CW in mW
* r: Setpoint CR in 0.1 Ohm
* v: Setpoint CV in mV
* E: Write settings to EEPROM. Only when settings are changed via the UI they are automatically written to EEPROM. Settings via the serial interface must be written using this command explicitly. However when the user changes any setting via the UI ALL settings are written to EEPROM.
* e: Read settings from EEPROM. This should be used after controlling the device via the serial interface to restore user's settings.
