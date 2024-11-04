//
// kernel.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef _kernel_h
#define _kernel_h

#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#include <circle/screen.h>
#include <circle/serial.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/types.h>
#include <circle/usb/usbcontroller.h>
#include <circle/usb/usbmidi.h>
#include <circle/usb/usbkeyboard.h>
#include "queue"
#include "vector"
#include "map"

#define SERIAL_BAUD 3000000

#define UART_RX_PIN 15
#define UART_TX_PIN 18

#define USB_GADGET_MODE

#define RET_PIN 23
#define YM_SENT_PIN 17
#define SCK_PIN 24
#define RST_PIN 18
#define BTN_PIN 3
#define OUT_MASK (BIT(RD_PIN) | BIT(WR_PIN) | BIT(RST_PIN))
#define D0_PIN 25
#define D1_PIN 8
#define D2_PIN 7
#define D3_PIN 1
#define D4_PIN 12
#define D5_PIN 16
#define D6_PIN 20
#define D7_PIN 21
#define DATA_MASK (BIT(D0_PIN) | BIT(D1_PIN) | BIT(D2_PIN) | BIT(D3_PIN) | BIT(D4_PIN) \
        | BIT(D5_PIN) | BIT(D6_PIN) | BIT(D7_PIN))
#define GET_BIT(value, n) ((value & BIT(n)) >> n)
#define TO_DATA_OUT(value) ((value & BIT(0)) << D0_PIN) \
              | (((value & BIT(1)) >> 1) << D1_PIN) \
              | (((value & BIT(2)) >> 2) << D2_PIN) \
              | (((value & BIT(3)) >> 3) << D3_PIN) \
              | (((value & BIT(4)) >> 4) << D4_PIN) \
              | (((value & BIT(5)) >> 5) << D5_PIN) \
              | (((value & BIT(6)) >> 6) << D6_PIN) \
              | (((value & BIT(7)) >> 7) << D7_PIN)
#define FROM_DATA_IN(value) ((value & BIT(D0_PIN)) >> D0_PIN) \
              | (((value & BIT(D1_PIN)) >> D1_PIN) << 1) \
              | (((value & BIT(D2_PIN)) >> D2_PIN) << 2) \
              | (((value & BIT(D3_PIN)) >> D3_PIN) << 3) \
              | (((value & BIT(D4_PIN)) >> D4_PIN) << 4) \
              | (((value & BIT(D5_PIN)) >> D5_PIN) << 5) \
              | (((value & BIT(D6_PIN)) >> D6_PIN) << 6) \
              | (((value & BIT(D7_PIN)) >> D7_PIN) << 7)


#define CMD_NOP                 0x00
#define CMD_RESET               0x0f
#define CMD_DEBUG               0x7f
#define CMD_YM_REGDATA          0x11
#define CMD_YM_CONFIG_2612      0x15

// system errors
#define ERROR_COMMAND_UNKNOWN   0xf1 // 11110001
#define ERROR_INVALID_STATE     0xf2 // 11110010
#define ERROR_TOO_MANY_BYTES    0xf3 // 11110011
// YM errors
#define ERROR_YM_IDX_OUTOFRANGE 0xf8 // 11111000
#define ERROR_YM_DOUBLE_SUBMIT  0xf9 // 11111001

#define YM_COUNT 2
#define YM_CHANNELS 6
#define SYNC_WRITE_LIMIT 100
#define QUEUE_SIZE_LIMIT 1000

#define MIDI_NOTE_OFF	0b1000
#define MIDI_NOTE_ON	0b1001
#define MIDI_CC		0b1011
#define MIDI_CC_VOLUME	7
#define KEY_NONE	255

enum TShutdownMode
{
    ShutdownNone,
    ShutdownHalt,
    ShutdownReboot
};

struct YMCommand {
    YMCommand(bool bank, u8 address, u8 data) {
        this->bank = bank;
        this->address = address;
        this->data = data;
    }
    bool bank;
    u8 address;
    u8 data;
};

struct YMQueue {
    bool sent = 0;
    std::queue<YMCommand> queue;
};

struct YMTimedNote {
    int step = 0;
    u16 note = 0;
};

struct YMSyncResult {
    u16 count = 0;
    u16 ones = 0;
    bool success = false;
};

struct TNoteInfo
{
	char	Key;
	u8	KeyNumber;	// MIDI number
};

struct PlayedNote
{
    u8  KeyNumber;
    u16 Frequency;
    bool KeyOn;
};

class CKernel
{
public:
    CKernel (void);
    ~CKernel (void);

    boolean Initialize (void);

    TShutdownMode Run (void);

    void YMTest();
    void YMPrepare (u8 chip, u8 channelIdx);
    void YMQueueData(u8 chip, u8 address, u8 data, bool bank = 0);
    u32 YMProcessQueue();
    u16 YMGetNote(u8 octave, u16 fnum);
    u16 YMQueueNote(u8 chip, u8 channel, PlayedNote note, bool prepare);
    u16 YMQueueNote(u8 chip, u8 channel, u16 frequency);
    void YMQueueNoteRaw(u8 chip, u8 channel, u16 note);
    void YMQueueNoteStop(u8 chip, u8 channel);
    void YMReset ();
    YMSyncResult SpinbusSync();
    bool YMWrite (u8 chip, YMCommand command);
    bool YMWrite (u8 chip, u8 address, u8 data, bool bank);
    bool WriteRead (u8 data);
    bool WriteReadRaw (u8 data);
    //u8 ReadData ();
    void DumpValue (u32 data, u8 len);
    void DumpError ();
    void ClearQueues ();
    bool RebootCheck ();

private:
	static void MIDIPacketHandler (unsigned nCable, u8 *pPacket, unsigned nLength);
	static void KeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6]);
	static void USBDeviceRemovedHandler (CDevice *pDevice, void *pContext);

    // do not change this order
    CActLED            m_ActLED;
    CKernelOptions        m_Options;
    CDeviceNameService    m_DeviceNameService;
    CScreenDevice        m_Screen;
    CSerialDevice        m_Serial;
    CExceptionHandler    m_ExceptionHandler;
    CInterruptSystem    m_Interrupt;
    CTimer            m_Timer;
    CLogger            m_Logger;
	CUSBController		*m_pUSB;
	//CUSBController		*m_pUSBGadget;
	CUSBMIDIDevice     * volatile m_pMIDIDevice;
	CUSBKeyboardDevice * volatile m_pKeyboard;

    // Button passthroughu
    CGPIOPin         m_BtnPin;

    // Reset and communication clock
    CGPIOPin        m_RSTPin;
    CGPIOPin        m_SCKPin;

    // 8-bit parallel send
    CGPIOPin        m_D0Pin;
    CGPIOPin        m_D1Pin;
    CGPIOPin        m_D2Pin;
    CGPIOPin        m_D3Pin;
    CGPIOPin        m_D4Pin;
    CGPIOPin        m_D5Pin;
    CGPIOPin        m_D6Pin;
    CGPIOPin        m_D7Pin;

    // 1-bit serial return
    CGPIOPin        m_RETPin;
    CGPIOPin        m_YMSentPin;

    std::map<u8, const char*> m_CommandMap {
        { CMD_NOP, "CMD_NOP" },
        { CMD_RESET, "CMD_RESET" },
        { CMD_DEBUG, "CMD_DEBUG" },
        { CMD_YM_REGDATA, "CMD_YM_REGDATA" },
        { CMD_YM_CONFIG_2612, "CMD_YM_CONFIG_2612" }
    };
    std::map<u8, const char*> m_ErrorMap {
        { ERROR_COMMAND_UNKNOWN, "ERROR_COMMAND_UNKNOWN" },
        { ERROR_INVALID_STATE, "ERROR_INVALID_STATE" },
        { ERROR_TOO_MANY_BYTES, "ERROR_TOO_MANY_BYTES" },
        { ERROR_YM_IDX_OUTOFRANGE, "ERROR_YM_IDX_OUTOFRANGE" },
        { ERROR_YM_DOUBLE_SUBMIT, "ERROR_YM_DOUBLE_SUBMIT" }
    };

    YMQueue queues[YM_COUNT];
    std::queue<PlayedNote> m_Notes;
    
    bool    m_Reading = false;
    bool    m_SentState = false;

    u8      m_PrevReadByte = 0;
    u8      m_LastReadByte = 0;
    u32     m_BytesRead = 0;
    u8      m_BitsRead = 0;
    u16      m_LastBits = 0;
    
    u8      m_ErrorData[8];
    u8      m_ErrorCode = 0;
    u8      m_ErrorLen = 0;
    u8      m_ErrorBytesRead = 0;
    bool    m_HasError = false;
    bool    m_Synchronized = false;

    u8      m_ChannelKeys[YM_COUNT*YM_CHANNELS] = { 0 };
    u8      m_LastChannelKeys[YM_COUNT*YM_CHANNELS] = { 0 };
    u8      m_NextChannel = 0;

    // MIDI control
	unsigned m_nFrequency;		// 0 if no key pressed
	unsigned m_nPrevFrequency;
	u8 m_ucKeyNumber;
	boolean m_bSetVolume;
	u8 m_uchVolume;
	static const float s_KeyFrequency[];
	static const TNoteInfo s_Keys[];
    
	static CKernel *s_pThis;
};

#endif
