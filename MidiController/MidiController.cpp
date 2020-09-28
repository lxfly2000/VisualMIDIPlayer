#include "MidiController.h"

MidiController::MidiController():hMidiIn(nullptr),fOnOpen(nullptr),fOnClose(nullptr),fOnData(nullptr),fOnMoreData(nullptr),fOnError(nullptr),
fOnLongData(nullptr),fOnLongError(nullptr)
{
}

MidiController::~MidiController()
{
}

MMRESULT MidiController::Init(unsigned deviceId,bool enableIoStatus)
{
	DWORD f = CALLBACK_FUNCTION;
	if (enableIoStatus)
		f |= MIDI_IO_STATUS;
	MMRESULT r = midiInOpen(&hMidiIn, deviceId, (DWORD_PTR)MidiController::MidiInProc, (DWORD_PTR)this, f);
	if (r == MMSYSERR_NOERROR)
		midiInStart(hMidiIn);
	return r;
}

MMRESULT MidiController::Uninit()
{
	midiInStop(hMidiIn);
	return midiInClose(hMidiIn);
}

void MidiController::MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	MidiController* pmc = (MidiController*)dwInstance;
	switch (wMsg)
	{
	case MIM_OPEN:
		if (pmc->fOnOpen)
			pmc->fOnOpen(0, 0);
		break;
	case MIM_CLOSE:
		if (pmc->fOnClose)
			pmc->fOnClose(0,0);
		break;
	case MIM_DATA:
		if (pmc->fOnData)
			pmc->fOnData(dwParam2,dwParam1);
		break;
	case MIM_MOREDATA:
		if (pmc->fOnMoreData)
			pmc->fOnMoreData(dwParam2, dwParam1);
		break;
	case MIM_ERROR:
		if (pmc->fOnError)
			pmc->fOnError(dwParam2, dwParam1);
		break;
	case MIM_LONGDATA:
		if (pmc->fOnLongData)
			pmc->fOnLongData(dwParam2, (LPMIDIHDR)dwParam1);
		break;
	case MIM_LONGERROR:
		if (pmc->fOnLongError)
			pmc->fOnLongError(dwParam2, (LPMIDIHDR)dwParam1);
		break;
	}
}

MidiController* MidiController::CreateMidiController(unsigned deviceId, bool enableIoStatus, MMRESULT* pmmr)
{
	MidiController* p = new MidiController();
	MMRESULT r = p->Init(deviceId,enableIoStatus);
	if (pmmr)
		*pmmr = r;
	if (r == MMSYSERR_NOERROR)
		return p;
	else
	{
		delete p;
		return nullptr;
	}
}

MMRESULT MidiController::ReleaseMidiController(MidiController* p)
{
	MMRESULT r = p->Uninit();
	if (r == MMSYSERR_NOERROR)
		delete p;
	return r;
}

HMIDIIN MidiController::GetHandle()
{
	return hMidiIn;
}

void MidiController::SetOnOpen(MIDICONTROLLER_CALLBACK f)
{
	fOnOpen = f;
}

void MidiController::SetOnClose(MIDICONTROLLER_CALLBACK f)
{
	fOnClose = f;
}

void MidiController::SetOnData(MIDICONTROLLER_CALLBACK f)
{
	fOnData = f;
}

void MidiController::SetOnMoreData(MIDICONTROLLER_CALLBACK f)
{
	fOnMoreData = f;
}

void MidiController::SetOnError(MIDICONTROLLER_CALLBACK f)
{
	fOnError = f;
}

void MidiController::SetOnLongData(MIDICONTROLLER_CALLBACK_LONG f)
{
	fOnLongData = f;
}

void MidiController::SetOnLongError(MIDICONTROLLER_CALLBACK_LONG f)
{
	fOnLongError = f;
}
