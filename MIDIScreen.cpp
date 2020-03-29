#include "MIDIScreen.h"

#define WHITEKEY_LENGTH 146
#define WHITEKEY_WIDTH 20
#define BLACKKEY_LENGTH 96
#define BLACKKEY_WIDTH 10
#define MAX_CHANNEL 16
#define ROW_SPACING 1
#define KEY_START 0
#define KEY_END 127


MIDIScreen::MIDIScreen() :colorWhiteKey(0x00001A80), colorBlackKey(0x00001A80),
colorWhiteKeyPressed(keyColors[0]), colorBlackKeyPressed(keyColors[0]), presentPressure(true), presentProgram(false)
{
	SetRectangle(0, 0, 640);
	ZeroMemory(chPrograms, ARRAYSIZE(chPrograms));
}

void MIDIScreen::Draw()
{
	if (!pplayer)return;
	DrawWhiteKey();
	DrawBlackKey();
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
}

int tempX, tempY;

void MIDIScreen::DrawWhiteKey()
{
	for (int j = MAX_CHANNEL - 1; j >= 0; j--)
		for (int i = KEY_END - KEY_START; i >= 0; i--)
			if (GetNumWhiteKey(i) != -1)
			{
				tempX = drawWidth_keyWhite*GetNumWhiteKey(i) + x;
				tempY = drawLength_keyWhite*j + y;
				SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
				DrawBox(tempX, tempY, tempX + drawWidth_keyWhite + 1, tempY + drawLength_keyWhite - ROW_SPACING, colorWhiteKey, FALSE);
				SetDrawBlendMode(DX_BLENDMODE_ALPHA, presentPressure ? 2 * pplayer->GetKeyPressure(j, i) : pplayer->GetKeyPressure(j, i) ? 255 : 0);
				tempX += (int)(pplayer->GetChannelPitchBend(j)*width_avgKey);
				DrawBox(tempX, tempY, tempX + drawWidth_keyWhite + 1, tempY + drawLength_keyWhite - ROW_SPACING, presentProgram ? keyColors[chPrograms[j] % 8] : colorWhiteKeyPressed, TRUE);
			}
}

void MIDIScreen::DrawBlackKey()
{
	for (int j = MAX_CHANNEL - 1; j >= 0; j--)
		for (int i = KEY_END - KEY_START; i >= 0; i--)
			if (GetNumBlackKey(i) != -1)
			{
				tempX = drawWidth_keyWhite*GetNumBlackKey(i) + start_keyBlackX;
				tempY = drawLength_keyWhite*j + y;
				SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
				DrawBox(tempX, tempY, tempX + drawWidth_keyBlack, tempY + drawLength_keyBlack, colorBlackKey, TRUE);
				SetDrawBlendMode(DX_BLENDMODE_ALPHA, presentPressure ? 2 * pplayer->GetKeyPressure(j, i) : pplayer->GetKeyPressure(j, i) ? 255 : 0);
				tempX += (int)(pplayer->GetChannelPitchBend(j)*width_avgKey);
				DrawBox(tempX, tempY, tempX + drawWidth_keyBlack, tempY + drawLength_keyBlack, presentProgram ? keyColors[chPrograms[j] % 8] : colorBlackKeyPressed, TRUE);
			}
}

int MIDIScreen::tableWhiteKey[] = {
	0,-1,1,-1,2,3,-1,4,-1,5,-1,6,
	7,-1,8,-1,9,10,-1,11,-1,12,-1,13,
	14,-1,15,-1,16,17,-1,18,-1,19,-1,20,
	21,-1,22,-1,23,24,-1,25,-1,26,-1,27,
	28,-1,29,-1,30,31,-1,32,-1,33,-1,34,
	35,-1,36,-1,37,38,-1,39,-1,40,-1,41,
	42,-1,43,-1,44,45,-1,46,-1,47,-1,48,
	49,-1,50,-1,51,52,-1,53,-1,54,-1,55,
	56,-1,57,-1,58,59,-1,60,-1,61,-1,62,
	63,-1,64,-1,65,66,-1,67,-1,68,-1,69,
	70,-1,71,-1,72,73,-1,74
};

int MIDIScreen::GetNumWhiteKey(int n, bool takeRightVar)
{
	if (takeRightVar)
		return tableWhiteKey[KEY_START + n] == -1 ? tableWhiteKey[KEY_START + n + 1] : tableWhiteKey[KEY_START + n];
	else return tableWhiteKey[KEY_START + n];
}

int MIDIScreen::tableBlackKey[] = {
	-1,0,-1,1,-1,-1,3,-1,4,-1,5,-1,
	-1,7,-1,8,-1,-1,10,-1,11,-1,12,-1,
	-1,14,-1,15,-1,-1,17,-1,18,-1,19,-1,
	-1,21,-1,22,-1,-1,24,-1,25,-1,26,-1,
	-1,28,-1,29,-1,-1,31,-1,32,-1,33,-1,
	-1,35,-1,36,-1,-1,38,-1,39,-1,40,-1,
	-1,42,-1,43,-1,-1,45,-1,46,-1,47,-1,
	-1,49,-1,50,-1,-1,52,-1,53,-1,54,-1,
	-1,56,-1,57,-1,-1,59,-1,60,-1,61,-1,
	-1,63,-1,64,-1,-1,66,-1,67,-1,68,-1,
	-1,70,-1,71,-1,-1,73,-1
};

int MIDIScreen::keyColors[8] = {
	0x00FFFF00,0x0000FF00,0x0000FFFF,0x008090FF,
	0x00FF00FF,0x00FF0000,0x00FF7F3F,0x00FF007F
};

int MIDIScreen::GetNumBlackKey(int n)
{
	return tableBlackKey[n];
}

void MIDIScreen::SetPlayerSrc(MidiPlayer * _pplayer)
{
	pplayer = _pplayer;
}

void MIDIScreen::SetRectangle(float _x, float _y, float _w, float _h)
{
	SetRectangle((int)_x, (int)_y, (int)_w, (int)_h);
}

void MIDIScreen::SetRectangle(int _x, int _y, int _w, int _h)
{
	x = _x;
	y = _y;
	w = _w;
	drawWidth_keyWhite = w / (GetNumWhiteKey(KEY_END, true) - GetNumWhiteKey(KEY_START, true) + 1);
	x += (_w - drawWidth_keyWhite*(GetNumWhiteKey(KEY_END, true) - GetNumWhiteKey(KEY_START, true) + 1)) / 2;
	if (_h)
		h = _h;
	else
		//������ʵĸ߶ȣ�7�鹲49���׼���16��
		h = w * MAX_CHANNEL * WHITEKEY_LENGTH / (WHITEKEY_WIDTH * (GetNumWhiteKey(KEY_END, true) -
			GetNumWhiteKey(KEY_START, true) + 1));
	x -= drawWidth_keyWhite*GetNumWhiteKey(KEY_START, true);
	drawLength_keyWhite = h / MAX_CHANNEL;
	y += (h - drawLength_keyWhite*MAX_CHANNEL) / 2;
	drawWidth_keyBlack = (drawWidth_keyWhite*BLACKKEY_WIDTH / WHITEKEY_WIDTH) | 1;
	start_keyBlackX = x + drawWidth_keyWhite - drawWidth_keyBlack / 2;
	drawLength_keyBlack = drawLength_keyWhite*BLACKKEY_LENGTH / WHITEKEY_LENGTH;
	width_avgKey = w / (KEY_END - KEY_START + 1);
}

void MIDIScreen::SetWhiteKeyColor(int color)
{
	colorWhiteKey = color;
}

void MIDIScreen::SetBlackKeyColor(int color)
{
	colorBlackKey = color;
}

void MIDIScreen::SetWhiteKeyPressedColor(int color)
{
	colorWhiteKeyPressed = color;
}

void MIDIScreen::SetBlackKeyPressedColor(int color)
{
	colorBlackKeyPressed = color;
}

void MIDIScreen::SetPresentPressure(bool b)
{
	presentPressure = b;
}