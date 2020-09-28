#pragma once

#include <Windows.h>

typedef void(CALLBACK* MIDICONTROLLER_CALLBACK)(DWORD_PTR timestamp_ms, DWORD_PTR midiMessage);
typedef void(CALLBACK* MIDICONTROLLER_CALLBACK_LONG)(DWORD_PTR timestamp_ms, LPMIDIHDR midiLongMessage);

class MidiController
{
private:
	MidiController();
	~MidiController();
	MMRESULT Init(unsigned deviceId, bool enableIoStatus);
	MMRESULT Uninit();
	HMIDIIN hMidiIn;
	static void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
	MIDICONTROLLER_CALLBACK fOnOpen,fOnClose,fOnError,fOnData,fOnMoreData;
	MIDICONTROLLER_CALLBACK_LONG fOnLongData, fOnLongError;
public:
	static MidiController* CreateMidiController(unsigned deviceId, bool enableIoStatus, MMRESULT* pmmr = nullptr);
	static MMRESULT ReleaseMidiController(MidiController* p);
	HMIDIIN GetHandle();
	void SetOnOpen(MIDICONTROLLER_CALLBACK f);
	void SetOnClose(MIDICONTROLLER_CALLBACK f);
	void SetOnData(MIDICONTROLLER_CALLBACK f);
	void SetOnMoreData(MIDICONTROLLER_CALLBACK f);
	void SetOnError(MIDICONTROLLER_CALLBACK f);
	void SetOnLongData(MIDICONTROLLER_CALLBACK_LONG f);
	void SetOnLongError(MIDICONTROLLER_CALLBACK_LONG f);
};
