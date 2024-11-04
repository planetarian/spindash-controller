#ifndef _usbminidexedmidigadget_h
#define _usbminidexedmidigadget_h

#if RASPPI==5
#include <circle/sysconfig.h>
#include <assert.h>

#warning No support for USB Gadget Mode on RPI 5 yet
class CUSBSpindashMIDIGadget
{
public:
	CUSBSpindashMIDIGadget (CInterruptSystem *pInterruptSystem)
	{
	}

	~CUSBSpindashMIDIGadget (void)
	{
	}
};
#else
#include <circle/usb/gadget/usbmidigadget.h>
#include <circle/usb/gadget/usbmidigadgetendpoint.h>
#include <circle/sysconfig.h>
#include <assert.h>

class CUSBSpindashMIDIGadget : public CUSBMIDIGadget
{
private:
#define MDSTRINGDESCRIPTORS 3
	const char *const s_SpindashStringDescriptor[MDSTRINGDESCRIPTORS] =
	{
		"\x04\x03\x09\x04",		// Language ID
		"planetarian",
		"Spindash"
	};

public:
	CUSBSpindashMIDIGadget (CInterruptSystem *pInterruptSystem)
	: CUSBMIDIGadget (pInterruptSystem)
	{
	}

	~CUSBSpindashMIDIGadget (void)
	{
		assert(0);
	}

protected:
	// Override GetDescriptor from CUSBMIDIGadget.
	// See CUSBMIDIGadget for details.
	// This will only act on the DESCRIPOR_STRING.
	// All other descriptors are returned from USBMIDIGadget.
	//
	const void *GetDescriptor (u16 wValue, u16 wIndex, size_t *pLength) override
	{
		assert (pLength);

		u8 uchDescIndex = wValue & 0xFF;

		switch (wValue >> 8)
		{
		case DESCRIPTOR_STRING:
			if (!uchDescIndex)
			{
				*pLength = (u8) s_SpindashStringDescriptor[0][0];
				return s_SpindashStringDescriptor[0];
			}
			else if (uchDescIndex < MDSTRINGDESCRIPTORS)
			{
				return CUSBMIDIGadget::ToStringDescriptor (s_SpindashStringDescriptor[uchDescIndex], pLength);
			}
			break;

		default:
			break;
		}

		return CUSBMIDIGadget::GetDescriptor(wValue, wIndex, pLength);
	}
};
#endif

#endif