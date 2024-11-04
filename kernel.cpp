//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2016  R. Stange <rsta2@o2online.de>
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
#include "kernel.h"
#include <circle/synchronize.h>
#include <circle/debug.h>
#include <circle/gpiopin.h>
#include <circle/timer.h>
#include <circle/usb/usbhcidevice.h>
#include <circle/usb/gadget/usbmidigadget.h>
#include <circle/machineinfo.h>
#include <assert.h>
#include <string.h>
#include "vector"
#include "queue"
#include "string"
#include "cmath"
#include "spindashgadget.h"


// See: http://www.deimos.ca/notefreqs/
const float CKernel::s_KeyFrequency[/* MIDI key number */] =
{
	8.17580, 8.66196, 9.17702, 9.72272, 10.3009, 10.9134, 11.5623, 12.2499, 12.9783, 13.7500,
	14.5676, 15.4339, 16.3516, 17.3239, 18.3540, 19.4454, 20.6017, 21.8268, 23.1247, 24.4997,
	25.9565, 27.5000, 29.1352, 30.8677, 32.7032, 34.6478, 36.7081, 38.8909, 41.2034, 43.6535,
	46.2493, 48.9994, 51.9131, 55.0000, 58.2705, 61.7354, 65.4064, 69.2957, 73.4162, 77.7817,
	82.4069, 87.3071, 92.4986, 97.9989, 103.826, 110.000, 116.541, 123.471, 130.813, 138.591,
	146.832, 155.563, 164.814, 174.614, 184.997, 195.998, 207.652, 220.000, 233.082, 246.942,
	261.626, 277.183, 293.665, 311.127, 329.628, 349.228, 369.994, 391.995, 415.305, 440.000,
	466.164, 493.883, 523.251, 554.365, 587.330, 622.254, 659.255, 698.456, 739.989, 783.991,
	830.609, 880.000, 932.328, 987.767, 1046.50, 1108.73, 1174.66, 1244.51, 1318.51, 1396.91,
	1479.98, 1567.98, 1661.22, 1760.00, 1864.66, 1975.53, 2093.00, 2217.46, 2349.32, 2489.02,
	2637.02, 2793.83, 2959.96, 3135.96, 3322.44, 3520.00, 3729.31, 3951.07, 4186.01, 4434.92,
	4698.64, 4978.03, 5274.04, 5587.65, 5919.91, 6271.93, 6644.88, 7040.00, 7458.62, 7902.13,
	8372.02, 8869.84, 9397.27, 9956.06, 10548.1, 11175.3, 11839.8, 12543.9
};

const TNoteInfo CKernel::s_Keys[] =
{
	{',', 72}, // C4
	{'M', 71}, // B4
	{'J', 70}, // A#4
	{'N', 69}, // A4
	{'H', 68}, // G#3
	{'B', 67}, // G3
	{'G', 66}, // F#3
	{'V', 65}, // F3
	{'C', 64}, // E3
	{'D', 63}, // D#3
	{'X', 62}, // D3
	{'S', 61}, // C#3
	{'Z', 60}  // C3
};

CKernel *CKernel::s_pThis = 0;

static const char FromKernel[] = "kernel";

CKernel::CKernel (void)
:    m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
    m_Timer (&m_Interrupt),
    m_Logger (m_Options.GetLogLevel (), &m_Timer),
#ifndef USB_GADGET_MODE
	m_pUSB (new CUSBHCIDevice (&m_Interrupt, &m_Timer, TRUE)), // TRUE: enable plug-and-play
#else
	m_pUSB (new CUSBSpindashMIDIGadget (&m_Interrupt)),
#endif
	m_pMIDIDevice (0),
	m_pKeyboard (0),
    m_BtnPin(BTN_PIN, GPIOModeInputPullDown),
    m_RSTPin(RST_PIN, GPIOModeOutput),
    m_SCKPin(SCK_PIN, GPIOModeOutput),
    m_D0Pin(D0_PIN, GPIOModeOutput),
    m_D1Pin(D1_PIN, GPIOModeOutput),
    m_D2Pin(D2_PIN, GPIOModeOutput),
    m_D3Pin(D3_PIN, GPIOModeOutput),
    m_D4Pin(D4_PIN, GPIOModeOutput),
    m_D5Pin(D5_PIN, GPIOModeOutput),
    m_D6Pin(D6_PIN, GPIOModeOutput),
    m_D7Pin(D7_PIN, GPIOModeOutput),
    m_RETPin(RET_PIN, GPIOModeInputPullDown),
    m_YMSentPin(YM_SENT_PIN, GPIOModeInputPullDown),
	m_nFrequency (0),
	m_nPrevFrequency (0),
	m_ucKeyNumber (KEY_NONE),
	m_bSetVolume (FALSE)
{
	s_pThis = this;
    m_ActLED.Blink (5);    // show we are alive
}

CKernel::~CKernel (void)
{
	s_pThis = 0;
}

boolean CKernel::Initialize (void)
{
    boolean bOK = TRUE;

    if (bOK)
    {
        bOK = m_Screen.Initialize ();
    }

    if (bOK)
    {
        bOK = m_Serial.Initialize (SERIAL_BAUD);
    }

    if (bOK)
    {
        CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
        if (pTarget == 0)
        {
            pTarget = &m_Screen;
        }

        bOK = m_Logger.Initialize (pTarget);
    }

    if (bOK)
    {
        bOK = m_Interrupt.Initialize ();
    }

    if (bOK)
    {
        bOK = m_Timer.Initialize ();
    }

	if (bOK)
	{
		assert (m_pUSB);
		bOK = m_pUSB->Initialize ();
	}

    return bOK;
}

TShutdownMode CKernel::Run (void)
{
    m_Logger.Write (FromKernel, LogNotice, "");
    m_Logger.Write (FromKernel, LogNotice, "Running...");

    bool run = true;
    while (run) {

        m_Logger.Write (FromKernel, LogNotice, "");
        m_Logger.Write (FromKernel, LogNotice, "Queueing YM prep instructions...");
        for (u8 chip = 0; chip < YM_COUNT; chip++) {
            for (u8 channelIdx = 0; channelIdx < 6; channelIdx++) {
                YMPrepare(chip, channelIdx);
            }
        }
        m_Logger.Write (FromKernel, LogNotice, "%d instructions queued per YM.", queues[0].queue.size());
        m_Logger.Write (FromKernel, LogNotice, "Resetting...");

        YMReset();
        
        // Wait for the YM to indicate it's ready to receive data
        m_Logger.Write (FromKernel, LogNotice, "Waiting for YM...");
        while (!m_YMSentPin.Read())
            m_Timer.nsDelay(1);
        
        
        // Try to synchronize with the chip by sending NOP
        // until we receive an 0x01 idle byte from the spinbus.
        m_Logger.Write (FromKernel, LogNotice, "Synchronizing...");

        YMSyncResult syncResult = SpinbusSync();

        if (!syncResult.success) {
            continue;
        }
        
        int remaining = 0;
        do
        {
            remaining = YMProcessQueue();
        } while (remaining > 0);

        if (m_ErrorLen > 0 && m_ErrorBytesRead >= m_ErrorLen)
        {
            DumpError();
            ClearQueues();
            m_Timer.MsDelay(1000);
            continue;
        }

        while (true) {
            m_Timer.usDelay(1);
		    bool bUpdated = m_pUSB->UpdatePlugAndPlay ();

            if (bUpdated) {
                // Update input devices
                if (m_pMIDIDevice == 0) {
                    m_pMIDIDevice =
                        (CUSBMIDIDevice *) CDeviceNameService::Get ()->GetDevice ("umidi1", FALSE);
                    if (m_pMIDIDevice != 0)
                    {
                        m_pMIDIDevice->RegisterRemovedHandler (USBDeviceRemovedHandler);
                        m_pMIDIDevice->RegisterPacketHandler (MIDIPacketHandler);
                    }
                }
                else if (m_pKeyboard == 0) {
                    m_pKeyboard =
                        (CUSBKeyboardDevice *) CDeviceNameService::Get ()->GetDevice ("ukbd1", FALSE);
                    if (m_pKeyboard != 0)
                    {
                        m_pKeyboard->RegisterRemovedHandler (USBDeviceRemovedHandler);
                        m_pKeyboard->RegisterKeyStatusHandlerRaw (KeyStatusHandlerRaw);
                    }
                }
            }

            // Play sounds
            while (m_Notes.size() > 0) {
                PlayedNote note = m_Notes.front();
                if (note.KeyOn) {
                    bool reuse = 0;
                    // find the last channel we used for this
                    for (u8 i = 0; i < YM_COUNT*YM_CHANNELS; i++) {
                        if (m_LastChannelKeys[i] == note.KeyNumber && m_ChannelKeys[i] == 0) {
                            // found it, reuse this channel
                            m_NextChannel = i;
                            reuse = true;
                            break;
                        }
                    }
                    if (!reuse) {
                        // find a free channel
                        for (u8 i = 0; i < YM_COUNT*YM_CHANNELS; i++) {
                            // check the key playing at the current channel index
                            u16 playingKey = m_ChannelKeys[m_NextChannel];
                            if (playingKey == 0) {
                                // we found an empty channel
                                break;
                            }
                            // check the next channel index
                            if (++m_NextChannel == YM_COUNT*YM_CHANNELS)
                                m_NextChannel = 0;
                        }
                    }
                    u8 chip = m_NextChannel/6;
                    u8 channel = m_NextChannel%6;
                    // if we didn't find it, just steal the channel we started on
                    if (m_ChannelKeys[m_NextChannel] != 0)
                        YMQueueNoteStop(chip, channel);
                    m_ChannelKeys[m_NextChannel] = note.KeyNumber;
                    m_LastChannelKeys[m_NextChannel] = note.KeyNumber;
                    //u16 noteShort =
                    YMQueueNote(chip, channel, note, !reuse);
                    //m_Logger.Write (FromKernel, LogNotice, "Note: chip %2d:%d %4dhz b:%d fnum:%4d (%04X)",
                        //chip+1, channel+1, note.Frequency, noteShort >> 11, noteShort & 0x7ff, noteShort & 0x7ff);
                    // prep for next note
                    if (++m_NextChannel == YM_COUNT*YM_CHANNELS)
                        m_NextChannel = 0;
                }
                else {
                    // find the channel the key is playing on
                    for (u8 i = 0; i < YM_COUNT*YM_CHANNELS; i++) {
                        if (m_ChannelKeys[i] == note.KeyNumber) {
                            m_ChannelKeys[i] = 0;
                            u8 chip = i/6;
                            u8 channel = i%6;
		                    //m_Logger.Write(FromKernel, LogDebug, "keyoff: %d, chip %d chan %d", note.KeyNumber, chip, channel);
                            YMQueueNoteStop(chip, channel);
                            break;
                        }
                    }
                }
                m_Notes.pop();
            }
            int remaining = 0;
            do
            {
                remaining = YMProcessQueue();
            } while (remaining > 0);
        }
    }
    
    m_Logger.Write (FromKernel, LogNotice, "Done.");

    return ShutdownReboot;
}

void CKernel::YMTest() {
    m_Logger.Write (FromKernel, LogNotice, "YM preparation complete. Playing notes...");

    #define NOTES_LENGTH 6
    u16 notes[] = {
        YMGetNote(4,1164),
        YMGetNote(4,872),
        YMGetNote(4,733),
        YMGetNote(3,1164),
        YMGetNote(3,872),
        YMGetNote(3,733)
    };

    #define TICKS_DIV 250000

    u8 chip = 0;
    u8 channel = 0;
    u64 ticks_cnt = 0;
    u64 ticks_last = m_Timer.GetClockTicks64();
    u8 note = 0;
    while (true) {
        m_Timer.nsDelay(1);


        if (m_ErrorLen > 0 && m_ErrorBytesRead >= m_ErrorLen)
        {
            DumpError();
            ClearQueues();
            break;
        }

        u64 ticks = m_Timer.GetClockTicks64();
        ticks_cnt += ticks - ticks_last;
        ticks_last = ticks;
        if (ticks_cnt < TICKS_DIV)
            continue;
        
        ticks_cnt -= TICKS_DIV;

        
        //YMReset();
        //SpinbusSync();
        YMPrepare(chip, channel);
        YMQueueNoteStop(chip, channel);
        YMQueueNoteRaw(chip, channel, notes[note]);
        u32 remaining;
        do {
            remaining = YMProcessQueue();
        } while (remaining > 0);
        m_Logger.Write (FromKernel, LogNotice, "Played note %d/%d (%d) for chip %d channel %d.", note+1, NOTES_LENGTH, notes[note], chip+1, channel+1);

        note = note == NOTES_LENGTH-1 ? 0 : note+1;
        if (++channel == YM_CHANNELS) {
            channel = 0;
            if (++chip == YM_COUNT) {
                chip = 0;
            }
        }

    }
}

void CKernel::YMPrepare (u8 chip, u8 channelIdx) {
    bool bank = channelIdx > 2;
        u8 chMod = channelIdx % 3;
    u8 channel = bank ? channelIdx + 1 : channelIdx;
    // global
    if (channelIdx == 0) {
        YMQueueData(chip, 0x22, 0x00); // LFO off
        //YMQueueData(chip, 0x24, 0x00); // Timer A Freq (high)
        //YMQueueData(chip, 0x25, 0x00); // Timer A Freq (low)
        //YMQueueData(chip, 0x26, 0x00); // Timer B Freq
        YMQueueData(chip, 0x27, 0x00); // Ch3 mode normal + timer off
        //YMQueueData(chip, 0x29, 0x00); // Ch6 DAC output
        YMQueueData(chip, 0x2B, 0x00); // Ch6 DAC off
        // jt12-specific registers
        // 0x00: ADPCMA_ON
        // 0x01: ADPCMA_TL
        // 0x02: ADPCMA_TEST
        // 0x21: TESTYM
        // 0x2C: DACTEST
        // 0x2D: CLK_N6
        // 0x2E: CLK_N3
        // 0x2F: CLK_N2
    }

    YMQueueData(chip, 0x28, channel);

    // operator
    YMQueueData(chip, 0x30+chMod, 0x71, bank); // OP1 DeTune / MULtiply (DT/MUL)
    YMQueueData(chip, 0x34+chMod, 0x0D, bank); // OP3
    YMQueueData(chip, 0x38+chMod, 0x33, bank); // OP2
    YMQueueData(chip, 0x3C+chMod, 0x01, bank); // OP4
    YMQueueData(chip, 0x40+chMod, 0x23, bank); // OP1 Total Level (TL)
    YMQueueData(chip, 0x44+chMod, 0x2D, bank); // OP3
    YMQueueData(chip, 0x48+chMod, 0x26, bank); // OP2
    YMQueueData(chip, 0x4C+chMod, 0x00, bank); // OP4
    YMQueueData(chip, 0x50+chMod, 0x5F, bank); // OP1 AttackRate/RateScale (AR/RS)
    YMQueueData(chip, 0x54+chMod, 0x99, bank); // OP3
    YMQueueData(chip, 0x58+chMod, 0x5F, bank); // OP2
    YMQueueData(chip, 0x5C+chMod, 0x92, bank); // OP4 (original 0x94)
    YMQueueData(chip, 0x60+chMod, 0x05, bank); // OP1 DecayRate / AmpMod Enable (DR[D1R]/AM)
    YMQueueData(chip, 0x64+chMod, 0x05, bank); // OP3
    YMQueueData(chip, 0x68+chMod, 0x05, bank); // OP2
    YMQueueData(chip, 0x6C+chMod, 0x07, bank); // OP4
    YMQueueData(chip, 0x70+chMod, 0x02, bank); // OP1 SustainRate (SR[D2R])
    YMQueueData(chip, 0x74+chMod, 0x02, bank); // OP3
    YMQueueData(chip, 0x78+chMod, 0x02, bank); // OP2
    YMQueueData(chip, 0x7C+chMod, 0x02, bank); // OP4
    YMQueueData(chip, 0x80+chMod, 0x11, bank); // OP1 ReleaseRate / SustainLevel (RR/SL)
    YMQueueData(chip, 0x84+chMod, 0x11, bank); // OP3
    YMQueueData(chip, 0x88+chMod, 0x11, bank); // OP2
    YMQueueData(chip, 0x8C+chMod, 0xA2, bank); // OP4 (def:A6)
    YMQueueData(chip, 0x90+chMod, 0x00, bank); // OP1 SSG-EG
    YMQueueData(chip, 0x94+chMod, 0x00, bank); // OP3
    YMQueueData(chip, 0x98+chMod, 0x00, bank); // OP2
    YMQueueData(chip, 0x9C+chMod, 0x00, bank); // OP4

    // Channel registers
    YMQueueData(chip, 0xA4+chMod, 0x22, bank); // block/fnum (high)
    YMQueueData(chip, 0xA0+chMod, 0x69, bank); // fnum (low)
    YMQueueData(chip, 0xB0+chMod, 0x32, bank); // Feedback/algorithm
    YMQueueData(chip, 0xB4+chMod, 0xC0, bank); // Pan/PMS/AMS
}

/// @brief gets the two-byte YM block+fnum value to send for selecting a note frequency
/// @param block 0-8: note block
/// @param fnum fnum for the note within the block
/// @return two-byte YM block+fnum note value
u16 CKernel::YMGetNote(u8 block, u16 fnum) {
  return ((block & 7) << 11) | (fnum & 0x7ff);
}

u16 CKernel::YMQueueNote(u8 chip, u8 channel, PlayedNote note, bool prepare) {
    if (note.KeyOn) {
        if (prepare)
            YMPrepare(chip, channel);
        return YMQueueNote(chip, channel, note.Frequency);
    }
    else {
        YMQueueNoteStop(chip, channel);
        return 0;
    }
}

u16 CKernel::YMQueueNote(u8 chip, u8 channel, u16 frequency) {
    u16 fnum = (144*(float)frequency*pow(2,20)/7669857) / 8;
    u8 block = 2;
    while (fnum >= 2048)
    {
        fnum /= 2;
        block++;
    }
    u16 note = ((block & 0x7) << 11) | (fnum & 0x07ff);
    YMQueueNoteRaw(chip, channel, note);
    return note;
}

void CKernel::YMQueueNoteRaw(u8 chip, u8 channel, u16 note) {
    YMQueueData(chip, 0xA4 + channel % 3, note >> 8, channel > 2); // block/fnum (high)
    YMQueueData(chip, 0xA0 + channel % 3, note & 0xff, channel > 2); // fnum (low)
    YMQueueData(chip, 0x28, 0xF0 + channel + (channel > 2 ? 1 : 0)); // Key on
}

void CKernel::YMQueueNoteStop(u8 chip, u8 channel) {
    YMQueueData(chip, 0x28, channel + (channel > 2 ? 1 : 0)); // Key off
}

void CKernel::YMReset () {
    m_RSTPin.Write(HIGH);
    m_Timer.usDelay(10);
    m_RSTPin.Write(LOW);
    m_Timer.usDelay(20);
    m_BitsRead = 0;
    m_LastReadByte = 0;
    m_PrevReadByte = 0;
}

YMSyncResult CKernel::SpinbusSync() {
    u8 count = 0;
    u8 ones = 0;
    do {
        ones += WriteReadRaw(CMD_NOP);
        if (m_LastBits == 0x0101) {
            m_LastReadByte = 1;
            m_PrevReadByte = 1;
            m_BitsRead = 0;
            break;
        }
    } while (++count < SYNC_WRITE_LIMIT);

    if (count == SYNC_WRITE_LIMIT) {
        m_Logger.Write (FromKernel, LogNotice, "Couldn't synchronize after %d writes. (last byte %04X)", count, m_LastBits);
        m_Timer.MsDelay(100);
        return { count, ones, false };
    }
    m_Logger.Write (FromKernel, LogNotice, "Synchronized after %d writes. (last byte %04X)", count, m_LastBits);
    return { count, ones, true };
}

void CKernel::YMQueueData(u8 chip, u8 address, u8 data, bool bank) {
    if (queues[chip].queue.size() > QUEUE_SIZE_LIMIT) {
        m_Logger.Write (FromKernel, LogNotice, "Hit queue limit (%d) for chip %d (addr %02X data %02X)", QUEUE_SIZE_LIMIT, chip, address, data);
        m_Timer.MsDelay(100);
    }
    else
        queues[chip].queue.emplace((bool)bank, address, data);
}

u32 CKernel::YMProcessQueue()
{
    // If the YM commands have been sent, reset the queues
    bool sentState = m_YMSentPin.Read();
    if (sentState) { // Sent flag rise
        for (int i = 0; i < YM_COUNT; i++)
            queues[i].sent = 0;
    }

    // Iterate through the send queues
    u32 remaining = 0;
    for (int i = 0; i < YM_COUNT; i++) {
        remaining += queues[i].queue.size();
        // if the YM data has been latched, we can now send to the same chips again
        // don't bother checking if there's nothing to send though
        if (!queues[i].queue.size() || queues[i].sent)
            continue;
            
        // send the command
        queues[i].sent = true;
        YMCommand cmd = queues[i].queue.front();
        YMWrite(i, cmd);
        queues[i].queue.pop();

        // send resulted in the completion of a return byte
        // YM reg-data commands are typically 4 bytes,
        // so we'll get a byte every two commands

        if (m_HasError) {
            ClearQueues();
            return 0;
        }
    }
    return remaining;
}

/// @brief Writes a YMCommand to the FPGA.
/// @param chip chip index to send the command to.
/// @param command YMCommand to sent.
/// @return true if the write resulted in the completion of a return data byte (stored in `m_LastReadByte`).
bool CKernel::YMWrite (u8 chip, YMCommand command)
{
    return YMWrite(chip, command.address, command.data, command.bank);
}

/// @brief Writes a YM command to the FPGA.
/// @param chip chip index to send the command to.
/// @param address address to write the data to.
/// @param data data to write.
/// @param bank 0: channels 1~3, 1: channels 4~6.
/// @return true if the write resulted in the completion of a return data byte (stored in `m_LastReadByte`).
bool CKernel::YMWrite (u8 chip, u8 address, u8 data, bool bank)
{
    bool byteCompleted = false;

    byteCompleted |= WriteRead(CMD_YM_REGDATA) << 3;
    byteCompleted |= WriteRead(chip << 1 | bank) << 2;
    byteCompleted |= WriteRead(address) << 1;
    byteCompleted |= WriteRead(data);

    return byteCompleted;
}


/// @brief Writes a byte of data to the FPGA, and shifts the return bit into m_LastBits.
/// @param data byte to write to the FPGA.
/// @return true if the write resulted in the completion of a return data byte (stored in `m_LastReadByte`).
bool CKernel::WriteRead (u8 data)
{
    WriteReadRaw(data);

    // check for error
    u8 errCnt = 0;
    if (!m_HasError && m_LastBits == 0xf0d4) {
        m_PrevReadByte = m_LastBits >> 8;
        m_LastReadByte = m_LastBits & 0xff;
        m_BitsRead = 0;
        m_HasError = true;
        errCnt = 32;
    }
    // found error
    while (m_HasError && errCnt > 0) {
        // read another byte
        for (u8 i = 0; i < 8; i++)
            WriteReadRaw(CMD_DEBUG);

        // third byte is the error code
        if (m_ErrorCode == 0) {
            m_ErrorCode = m_LastReadByte;
        }
        // fourth byte is the error data length
        else if (m_ErrorLen == 0) {
            m_ErrorLen = m_LastReadByte;
            if (m_ErrorLen > 8)
                m_ErrorLen = 8;
        }
        // remaining bytes are error data
        else if (m_ErrorBytesRead < m_ErrorLen) {
            m_ErrorData[m_ErrorBytesRead++] = m_LastReadByte;
        }
        
        if (m_ErrorLen > 0 && m_ErrorBytesRead >= m_ErrorLen)  {
            DumpError();
            return 0;
        }
    }
    if (++m_BitsRead == 8)
    {
        m_BitsRead = 0;
        m_PrevReadByte = m_LastBits >> 8;
        m_LastReadByte = m_LastBits & 0xff;
        
        if (m_LastReadByte != 1) {
            YMSyncResult syncResult = SpinbusSync();
            m_Synchronized = syncResult.success;
            if (!syncResult.success)
                return false;
        }
        return true;
    }
    return false;
}

bool CKernel::WriteReadRaw(u8 data) {
    u32 outWord = TO_DATA_OUT(data);
    CGPIOPin::WriteAll(outWord, DATA_MASK);
    m_Timer.nsDelay(40);
    m_SCKPin.Write(HIGH);
    m_Timer.nsDelay(40);
    m_SCKPin.Write(LOW);

    bool bit = m_RETPin.Read();
    m_LastBits <<= 1;
    m_LastBits |= bit;
    return bit;
}

std::string hexStr(unsigned char *data, int len)
{
  static const char hex_digits[] = "0123456789ABCDEF";
  std::string s(len * 3, ' ');
  for (int i = 0; i < len; ++i) {
    s[3 * i]     = hex_digits[(data[i] & 0xF0) >> 4];
    s[3 * i + 1] = hex_digits[data[i] & 0x0F];
  }
  return s;
}

void CKernel::DumpError ()
{
    m_Logger.Write(FromKernel, LogError, "");
    m_Logger.Write(FromKernel, LogError, "SPINDASH ERROR");
    if (auto search = m_ErrorMap.find(m_ErrorCode); search != m_ErrorMap.end())
        m_Logger.Write(FromKernel, LogError, search->second);
    m_Logger.Write(FromKernel, LogError, "ERRCODE: %02X", m_ErrorCode);
    for (int i = 0; i < m_ErrorLen; i++)
        m_Logger.Write(FromKernel, LogError, "DATA %2d: %02X", i, m_ErrorData[i]);

    switch (m_ErrorCode) {
        case ERROR_COMMAND_UNKNOWN:
            m_Logger.Write(FromKernel, LogError, "Unknown command received: %02X", m_ErrorData[0]);
            break;
        case ERROR_INVALID_STATE:
            m_Logger.Write(FromKernel, LogError, "Invalid command receiver state: %02X", m_ErrorData[1]);
            break;
        case ERROR_TOO_MANY_BYTES:
            m_Logger.Write(FromKernel, LogError, "Received too many bytes for command: %02X", m_ErrorData[0]);
            break;
        case ERROR_YM_IDX_OUTOFRANGE:
            m_Logger.Write(FromKernel, LogError, "YM chip index out of range: %d", m_ErrorData[1] >> 1);
            break;
        case ERROR_YM_DOUBLE_SUBMIT:
            m_Logger.Write(FromKernel, LogError, "YM double submission on chip index: %d", m_ErrorData[1] >> 1);
            break;
        default:
            m_Logger.Write(FromKernel, LogError, "Unknown error code: %02X", m_ErrorCode);
            break;
    }

    
    m_HasError = false;
    m_ErrorCode = 0;
    m_ErrorLen = 0;
    m_ErrorBytesRead = 0;

    m_LastReadByte = 0;
    m_PrevReadByte = 0;
    m_BitsRead = 0;
    m_BytesRead = 0;

    m_SentState = false;
    m_Reading = false;
}

void CKernel::ClearQueues () {
    for (int i = 0; i < YM_COUNT; i++) {
        std::queue<YMCommand> empty;
        std::swap(queues[i].queue, empty);
    }
}

void CKernel::DumpValue (u32 data, u8 len)
{
    char buf[len+1];
    for (u8 i = 0; i < len; i++)
    {
        buf[(len-1)-i] = (data & BIT(i)) != 0 ? '1' : '0';
    }
    buf[len] = '\0';
    
    m_Logger.Write (FromKernel, LogNotice, buf, len+1);
}

bool CKernel::RebootCheck()
{
    char rebootMagic[] = "tAgHQP3Lw2NZcW8Uru7jnf";
    char serialBuf[23];
    u32 serialBytes = m_Serial.Read(serialBuf, sizeof serialBuf);
    if (serialBytes > 0)
    {
        m_Logger.Write (FromKernel, LogNotice, "Data received");
        m_Logger.Write (FromKernel, LogNotice, serialBuf);
        if (strcmp(rebootMagic, serialBuf) == 0)
            return true;
        m_Timer.MsDelay(200);
    }
    return false;
}

void CKernel::MIDIPacketHandler (unsigned nCable, u8 *pPacket, unsigned nLength)
{
	assert (s_pThis != 0);

	// The packet contents are just normal MIDI data - see
	// https://www.midi.org/specifications/item/table-1-summary-of-midi-message

	if (nLength < 3)
	{
		return;
	}

	u8 ucStatus    = pPacket[0];
	//u8 ucChannel   = ucStatus & 0x0F;
	u8 ucType      = ucStatus >> 4;
	u8 ucKeyNumber = pPacket[1];
	u8 ucVelocity  = pPacket[2];

    //CLogger::Get()->Write(FromKernel, LogDebug, "MIDI event: %02X %02X %02X", pPacket[0], pPacket[1], pPacket[2]);

	if (ucType == MIDI_NOTE_ON && ucVelocity > 0)
	{
		if (   ucVelocity > 0
		    && ucKeyNumber < sizeof s_KeyFrequency / sizeof s_KeyFrequency[0])
		{
			s_pThis->m_ucKeyNumber = ucKeyNumber;
			s_pThis->m_nFrequency = (unsigned) (s_KeyFrequency[ucKeyNumber]);
            

            u16 freq = s_KeyFrequency[ucKeyNumber];
            s_pThis->m_Notes.push({ucKeyNumber, freq, true});
		}
		else
		{
			if (s_pThis->m_ucKeyNumber == ucKeyNumber)
			{
				s_pThis->m_ucKeyNumber = KEY_NONE;
				s_pThis->m_nFrequency = 0;
			}
		}
	}
	else if (ucType == MIDI_NOTE_OFF || (ucType == MIDI_NOTE_ON && ucVelocity == 0))
	{
        u16 freq = s_KeyFrequency[ucKeyNumber];
            s_pThis->m_Notes.push({ucKeyNumber, freq, false});
		if (s_pThis->m_ucKeyNumber == ucKeyNumber)
		{
			s_pThis->m_ucKeyNumber = KEY_NONE;
			s_pThis->m_nFrequency = 0;
		}
	}
	else if (ucType == MIDI_CC)
	{
		if (pPacket[1] == MIDI_CC_VOLUME)
		{
			s_pThis->m_uchVolume = pPacket[2];
			s_pThis->m_bSetVolume = TRUE;
		}
	}
}

void CKernel::KeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6])
{
	assert (s_pThis != 0);

	// find the key code of a pressed key
	char chKey = '\0';
	for (unsigned i = 0; i <= 5; i++)
	{
		u8 ucKeyCode = RawKeys[i];
		if (ucKeyCode != 0)
		{
			if (0x04 <= ucKeyCode && ucKeyCode <= 0x1D)
			{
				chKey = RawKeys[i]-0x04+'A';	// key code of 'A' is 0x04
			}
			else if (ucKeyCode == 0x36)
			{
				chKey = ',';			// key code of ',' is 0x36
			}

			break;
		}
	}

	if (chKey != '\0')
	{
		// find the pressed key in the key table and set its frequency
		for (unsigned i = 0; i < sizeof s_Keys / sizeof s_Keys[0]; i++)
		{
			if (s_Keys[i].Key == chKey)
			{
				u8 ucKeyNumber = s_Keys[i].KeyNumber;

				assert (ucKeyNumber < sizeof s_KeyFrequency / sizeof s_KeyFrequency[0]);
				s_pThis->m_nFrequency = (unsigned) (s_KeyFrequency[ucKeyNumber] + 0.5);

				return;
			}
		}
	}

	s_pThis->m_nFrequency = 0;
}

void CKernel::USBDeviceRemovedHandler (CDevice *pDevice, void *pContext)
{
	assert (s_pThis != 0);

	if (s_pThis->m_pMIDIDevice == (CUSBMIDIDevice *) pDevice)
	{
		CLogger::Get()->Write(FromKernel, LogDebug, "USB MIDI keyboard removed");

		s_pThis->m_pMIDIDevice = 0;
	}
	else if (s_pThis->m_pKeyboard == (CUSBKeyboardDevice *) pDevice)
	{
		CLogger::Get()->Write(FromKernel, LogDebug, "USB PC keyboard removed");

		s_pThis->m_pKeyboard = 0;
	}
}