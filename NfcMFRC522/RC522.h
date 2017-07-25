#pragma once

enum PCDCommand : BYTE
{
	PCDCommand_Idle = 0x0,
	PCDCommand_Mem = 0x1,
	PCDCommand_GenerateRandomId = 0x2,
	PCDCommand_CalcCRC = 0x3,
	PCDCommand_Transmit = 0x4,
	PCDCommand_NoCmdChange = 0x7,
	PCDCommand_Receive = 0x8,
	PCDCommand_Transceive = 0xC,
	PCDCommand_MFAuthent = 0xE,
	PCDCommand_SoftReset = 0xF
};

enum PCDRegister : BYTE
{
	PCDRegister_CommandReg = 0x01,  // starts and stops command execution
	PCDRegister_ComIEnReg = 0x02,  // enable and disable interrupt request control bits
	PCDRegister_DivIEnReg = 0x03,  // enable and disable interrupt request control bits
	PCDRegister_ComIrqReg = 0x04,  // interrupt request bits
	PCDRegister_DivIrqReg = 0x05,  // interrupt request bits
	PCDRegister_ErrorReg = 0x06,  // error bits showing the error status of the last command executed
	PCDRegister_Status1Reg = 0x07,  // communication status bits
	PCDRegister_Status2Reg = 0x08,  // receiver and transmitter status bits
	PCDRegister_FIFODataReg = 0x09,  // input and output of 64 byte FIFO buffer
	PCDRegister_FIFOLevelReg = 0x0A,  // number of bytes stored in the FIFO buffer
	PCDRegister_WaterLevelReg = 0x0B,  // level for FIFO underflow and overflow warning
	PCDRegister_ControlReg = 0x0C,  // miscellaneous control registers
	PCDRegister_BitFramingReg = 0x0D,  // adjustments for bit-oriented frames
	PCDRegister_CollReg = 0x0E,  // bit position of the first bit-collision detected on the RF interface
	PCDRegister_ModeReg = 0x11,  // defines general modes for transmitting and receiving
	PCDRegister_TxModeReg = 0x12,  // defines transmission data rate and framing
	PCDRegister_RxModeReg = 0x13,  // defines reception data rate and framing
	PCDRegister_TxControlReg = 0x14,  // controls the logical behavior of the antenna driver pins TX1 and TX2
	PCDRegister_TxASKReg = 0x15,  // controls the setting of the transmission modulation
	PCDRegister_TxSelReg = 0x16,  // selects the internal sources for the antenna driver
	PCDRegister_RxSelReg = 0x17,  // selects internal receiver settings
	PCDRegister_RxThresholdReg = 0x18,  // selects thresholds for the bit decoder
	PCDRegister_DemodReg = 0x19,  // defines demodulator settings
	PCDRegister_MfTxReg = 0x1C,  // controls some MIFARE communication transmit parameters
	PCDRegister_MfRxReg = 0x1D,  // controls some MIFARE communication receive parameters
	PCDRegister_SerialSpeedReg = 0x1F,  // selects the speed of the serial UART interface
	PCDRegister_CRCResultRegH = 0x21,  // shows the MSB and LSB values of the CRC calculation
	PCDRegister_CRCResultRegL = 0x22,
	PCDRegister_ModWidthReg = 0x24,  // controls the ModWidth setting?
	PCDRegister_RFCfgReg = 0x26,  // configures the receiver gain
	PCDRegister_GsNReg = 0x27,  // selects the conductance of the antenna driver pins TX1 and TX2 for modulation
	PCDRegister_CWGsPReg = 0x28,  // defines the conductance of the p-driver output during periods of no modulation
	PCDRegister_ModGsPReg = 0x29,  // defines the conductance of the p-driver output during periods of modulation
	PCDRegister_TModeReg = 0x2A,  // defines settings for the internal timer
	PCDRegister_TPrescalerReg = 0x2B,  // the lower 8 bits of the TPrescaler value. The 4 high bits are in TModeReg.
	PCDRegister_TReloadRegH = 0x2C,  // defines the 16-bit timer reload value
	PCDRegister_TReloadRegL = 0x2D,
	PCDRegister_TCntValueRegH = 0x2E,  // shows the 16-bit timer value
	PCDRegister_TCntValueRegL = 0x2F,
	PCDRegister_TestSel1Reg = 0x31,  // general test signal configuration
	PCDRegister_TestSel2Reg = 0x32,  // general test signal configuration
	PCDRegister_TestPinEnReg = 0x33,  // enables pin output driver on pins D1 to D7
	PCDRegister_TestPinValueReg = 0x34,  // defines the values for D1 to D7 when it is used as an I/O bus
	PCDRegister_TestBusReg = 0x35,  // shows the status of the internal test bus
	PCDRegister_AutoTestReg = 0x36,  // controls the digital self test
	PCDRegister_VersionReg = 0x37,  // shows the software version
	PCDRegister_AnalogTestReg = 0x38,  // controls the pins AUX1 and AUX2
	PCDRegister_TestDAC1Reg = 0x39,  // defines the test value for TestDAC1
	PCDRegister_TestDAC2Reg = 0x3A,  // defines the test value for TestDAC2
	PCDRegister_TestADCReg = 0x3B   // shows the value of ADC I and Q channels
};

enum PICCCommand : BYTE
{
	PICCCommand_ReqA = 0x26,		// REQuest command, Type A. Invites PICCs in state IDLE to go to READY and prepare for anticollision or selection. 7 bit frame.
	PICCCommand_WUpA = 0x52,		// Wake-UP command, Type A. Invites PICCs in state IDLE and HALT to go to READY(*) and prepare for anticollision or selection. 7 bit frame.
	PICCCommand_CT = 0x88,		// Cascade Tag. Not really a command, but used during anti collision.
	PICCCommand_SelCL1 = 0x93,		// Anti collision/Select, Cascade Level 1
	PICCCommand_SelCL2 = 0x95,		// Anti collision/Select, Cascade Level 1
	PICCCommand_SelCL3 = 0x97,		// Anti collision/Select, Cascade Level 1
	PICCCommand_HaLT = 0x50,		// HaLT command, Type A. Instructs an ACTIVE PICC to go to state HALT.
	PICCCommand_MIFAREAuthKeyA = 0x60,		// Perform authentication with Key A
	PICCCommand_MIFAREAuthKeyB = 0x61,		// Perform authentication with Key B
	PICCCommand_MIFARERead = 0x30,		// Reads one 16 byte block from the authenticated sector of the PICC. Also used for MIFARE Ultralight.
	PICCCommand_MIFAREWrite = 0xA0,		// Writes one 16 byte block to the authenticated sector of the PICC. Called "COMPATIBILITY WRITE" for MIFARE Ultralight.
	PICCCommand_MIFAREDecrement = 0xC0,		// Decrements the contents of a block and stores the result in the internal data register.
	PICCCommand_MIFAREIncrement = 0xC1,		// Increments the contents of a block and stores the result in the internal data register.
	PICCCommand_MIFARERestore = 0xC2,		// Reads the contents of a block into the internal data register.
	PICCCommand_MIFARETransfer = 0xB0,		// Writes the contents of the internal data register to a block.
	PICCCommand_ULWrite = 0xA2,		// Writes one 4 byte page to the PICC.
};

enum AntennaGain : BYTE
{
	AntennaGain_dB18 = 2,
	AntennaGain_db23 = 3,
	AntennaGain_db33 = 4,
	AntennaGain_db38 = 5,
	AntennaGain_db43 = 6,
	AntennaGain_db48 = 7
};

enum StatusCode
{
	StatusCode_OK = 1,
	StatusCode_Error = 2,
	StatusCode_Collision = 3,
	StatusCode_Timeout = 4,
	StatusCode_BufferTooSmall = 5,
	StatusCode_InternalError = 6,
	StatusCode_InvalidArgument = 7,
	StatusCode_WrongCRC = 8,
	StatusCode_MiFareNAK = 9
};