/* ########################################################################

   PICSimLab - Programmable IC Simulator Laboratory

   ########################################################################

   Copyright (c) : 2019-2023  Luis Claudio Gambôa Lopes <lcgamboa@yahoo.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   For e-mail suggestions :  lcgamboa@yahoo.com
   ######################################################################## */

#include "output_Buzzer.h"
#include "../lib/oscilloscope.h"
#include "../lib/picsimlab.h"
#include "../lib/spareparts.h"

/* outputs */
enum { O_P1, O_P2, O_L1 };

enum { ACTIVE = 0, PASSIVE, TONE };

static PCWProp pcwprop[5] = {{PCW_COMBO, "Pin 1"},
                             {PCW_LABEL, "Pin2,GND"},
                             {PCW_COMBO, "Type"},
                             {PCW_COMBO, "Active"},
                             {PCW_END, ""}};

cpart_Buzzer::cpart_Buzzer(const unsigned x, const unsigned y, const char* name, const char* type, board* pboard_)
    : part(x, y, name, type, pboard_), font(9, lxFONTFAMILY_TELETYPE, lxFONTSTYLE_NORMAL, lxFONTWEIGHT_BOLD) {
    X = x;
    Y = y;
    active = 1;
    always_update = 1;

    ReadMaps();

    LoadImage();

    input_pins[0] = 0;

    mcount = 0;

    buzzer.Init();
    btype = ACTIVE;

    samplerate = buzzer.GetSampleRate();
    buffersize = samplerate / 5;  // 0.1 seconds
    buffer = NULL;
    maxv = buzzer.GetMax();
    buffercount = 0;
    ctone = 0;
    ftone = 0;
    optone = 0;
    oftone = 0;

    in[0] = 0;
    in[1] = 0;
    in[2] = 0;

    out[0] = 0;
    out[1] = 0;
    out[2] = 0;

    if (PICSimLab.GetWindow()) {
        timer = (CTimer*)PICSimLab.GetWindow()->GetChildByName("timer1");
    }
    SetPCWProperties(pcwprop);

    PinCount = 1;
    Pins = input_pins;
}

cpart_Buzzer::~cpart_Buzzer(void) {
    delete Bitmap;
    if (buffer) {
        delete[] buffer;
    }
    canvas.Destroy();
    buzzer.End();
}

void cpart_Buzzer::DrawOutput(const unsigned int i) {
    const picpin* ppins = SpareParts.GetPinsValues();

    canvas.SetFgColor(30, 0, 0);

    switch (output[i].id) {
        case O_P1:
            canvas.SetColor(49, 61, 99);
            canvas.Rectangle(1, output[i].x1, output[i].y1, output[i].x2 - output[i].x1, output[i].y2 - output[i].y1);
            canvas.SetFgColor(255, 255, 255);
            if (input_pins[output[i].id - O_P1] == 0)
                canvas.RotatedText("NC", output[i].x1, output[i].y1, 0);
            else
                canvas.RotatedText(SpareParts.GetPinName(input_pins[output[i].id - O_P1]), output[i].x1, output[i].y1,
                                   0);
            break;
        case O_P2:
            canvas.SetColor(49, 61, 99);
            canvas.Rectangle(1, output[i].x1, output[i].y1, output[i].x2 - output[i].x1, output[i].y2 - output[i].y1);
            canvas.SetFgColor(255, 255, 255);
            canvas.RotatedText("GND", output[i].x1, output[i].y1, 0);
            break;
        case O_L1:
            canvas.SetFgColor(0, 0, 0);
            color1.Set(55, 0, 0);

            if (input_pins[0] > 0) {
                if (active) {
                    color1.Set(ppins[input_pins[0] - 1].oavalue, 0, 0);
                } else {
                    color1.Set(310 - ppins[input_pins[0] - 1].oavalue, 0, 0);
                }
            }
            // draw a LED
            canvas.SetBgColor(color1);
            int r = color1.Red() - 120;
            int g = color1.Green() - 120;
            int b = color1.Blue() - 120;
            if (r < 0)
                r = 0;
            if (g < 0)
                g = 0;
            if (b < 0)
                b = 0;
            color2.Set(r, g, b);
            canvas.SetBgColor(color2);
            canvas.Circle(1, output[i].x1, output[i].y1, output[i].r + 1);
            canvas.SetBgColor(color1);
            canvas.Circle(1, output[i].x1, output[i].y1, output[i].r - 2);
            break;
    }
}

unsigned short cpart_Buzzer::GetInputId(char* name) {
    printf("Error input '%s' don't have a valid id! \n", name);
    return INVALID_ID;
}

unsigned short cpart_Buzzer::GetOutputId(char* name) {
    if (strcmp(name, "PN_1") == 0)
        return O_P1;
    if (strcmp(name, "PN_2") == 0)
        return O_P2;
    if (strcmp(name, "LD_1") == 0)
        return O_L1;

    printf("Error output '%s' don't have a valid id! \n", name);
    return INVALID_ID;
};

lxString cpart_Buzzer::WritePreferences(void) {
    char prefs[256];

    sprintf(prefs, "%hhu,%hhu,%hhu", input_pins[0], btype, active);
    return prefs;
}

void cpart_Buzzer::ReadPreferences(lxString value) {
    unsigned char tp;
    sscanf(value.c_str(), "%hhu,%hhu,%hhu", &input_pins[0], &tp, &active);
    ChangeType(tp);
}

void cpart_Buzzer::RegisterRemoteControl(void) {
    const picpin* ppins = SpareParts.GetPinsValues();

    output_ids[O_L1]->status = (void*)&ppins[input_pins[0] - 1].oavalue;
}

void cpart_Buzzer::ConfigurePropertiesWindow(CPWindow* WProp) {
    SetPCWComboWithPinNames(WProp, "combo1", input_pins[0]);

    ((CCombo*)WProp->GetChildByName("combo3"))->SetItems("Active,Passive,Tone,");
    if (btype == ACTIVE)
        ((CCombo*)WProp->GetChildByName("combo3"))->SetText("Active");
    else if (btype == PASSIVE) {
        ((CCombo*)WProp->GetChildByName("combo3"))->SetText("Passive");
    } else  // TONE
    {
        ((CCombo*)WProp->GetChildByName("combo3"))->SetText("Tone");
    }

    ((CCombo*)WProp->GetChildByName("combo4"))->SetItems("HIGH,LOW,");
    if (active)
        ((CCombo*)WProp->GetChildByName("combo4"))->SetText("HIGH");
    else
        ((CCombo*)WProp->GetChildByName("combo4"))->SetText("LOW ");
}

void cpart_Buzzer::ReadPropertiesWindow(CPWindow* WProp) {
    input_pins[0] = GetPWCComboSelectedPin(WProp, "combo1");

    unsigned char tp = 0;

    lxString mode = ((CCombo*)WProp->GetChildByName("combo3"))->GetText();
    if (mode.compare(lxT("Active")) == 0) {
        tp = ACTIVE;
    } else if (mode.compare(lxT("Passive")) == 0) {
        tp = PASSIVE;
    } else {  // TONE
        tp = TONE;
    }

    active = (((CCombo*)WProp->GetChildByName("combo4"))->GetText().compare("HIGH") == 0);

    ChangeType(tp);
}

void cpart_Buzzer::PreProcess(void) {
    if (btype == PASSIVE) {
        JUMPSTEPS_ = (pboard->MGetInstClockFreq() / samplerate);
        JUMPSTEPS_ *= ((float)BASETIMER) / timer->GetTime();  // Adjust to sample at the same time to the timer
        mcount = JUMPSTEPS_;
    } else if (btype == TONE) {
        JUMPSTEPS_ = (pboard->MGetInstClockFreq() / samplerate);
        mcount = JUMPSTEPS_;
        ctone = 0;
        optone = 0;
        ftone = 0;
    }
}

void cpart_Buzzer::Stop(void) {
    if ((btype == ACTIVE) || (btype == TONE)) {
        buzzer.BeepStop();
    }
}

void cpart_Buzzer::Process(void) {
    if (btype == PASSIVE) {
        mcount++;
        if (mcount > JUMPSTEPS_) {
            if ((input_pins[0]) && (buffercount < buffersize)) {
                const picpin* ppins = SpareParts.GetPinsValues();

                /*
                   0.7837 z-1 - 0.7837 z-2
             y1:  ----------------------
                  1 - 1.196 z-1 + 0.2068 z-2
                 */
                in[2] = in[1];
                in[1] = in[0];
                if (active) {
                    in[0] = ((2.0 * ppins[input_pins[0] - 1].value) - 1.0) * maxv * 0.5;
                } else {
                    in[0] = ((2.0 * (ppins[input_pins[0] - 1].value == 0)) - 1.0) * maxv * 0.5;
                }
                out[2] = out[1];
                out[1] = out[0];
                out[0] = 0.7837 * in[1] - 0.7837 * in[2] + 1.196 * out[1] - 0.2068 * out[2];

                buffer[buffercount++] = out[0];
            }

            mcount = 0;
        }
    } else if (btype == TONE) {
        mcount++;
        if (mcount > JUMPSTEPS_) {
            if (input_pins[0]) {
                const picpin* ppins = SpareParts.GetPinsValues();

                if ((!optone) && (ppins[input_pins[0] - 1].value)) {
                    ftone = (ftone + ctone) / 2.0;
                    ctone = 0;
                }
                optone = ppins[input_pins[0] - 1].value;
                ctone++;
            }
            mcount = 0;
        }
    }
}

void cpart_Buzzer::PostProcess(void) {
    const picpin* ppins = SpareParts.GetPinsValues();

    if (btype == ACTIVE) {
        if (active) {
            if (ppins[input_pins[0] - 1].oavalue > 65) {
                buzzer.BeepStart();
            } else {
                buzzer.BeepStop();
            }
        } else {
            if ((310 - ppins[input_pins[0] - 1].oavalue) > 215) {
                buzzer.BeepStart();
            } else {
                buzzer.BeepStop();
            }
        }
    } else if (btype == PASSIVE) {
        buffer[buffercount - 1] = 0;
        int ret = buzzer.SoundPlay(buffer, buffercount);
        printf("ret=%i buffercount=%i sample=%i time=%f timer=%i\n", ret, buffercount, samplerate,
               ((float)(buffercount)) / samplerate, timer->GetTime());
        buffercount = 0;
    } else  // TONE
    {
        float freq;

        if (ftone > 5) {
            freq = samplerate / ftone;
        } else {
            freq = 0;
        }

        if (freq > 100) {
            if (fabs(oftone - freq) > 10.0) {
                buzzer.BeepStop();
                buzzer.BeepStart(freq, 0.5, 1);
                oftone = freq;
            }
        } else {
            buzzer.BeepStop();
            oftone = 0;
        }
    }

    if (input_pins[0] && (output_ids[O_L1]->value != ppins[input_pins[0] - 1].oavalue)) {
        output_ids[O_L1]->value = ppins[input_pins[0] - 1].oavalue;
        output_ids[O_L1]->update = 1;
    }
}

void cpart_Buzzer::ChangeType(unsigned char tp) {
    if (btype == tp)
        return;

    if ((btype == ACTIVE) || (btype == TONE)) {
        buzzer.BeepStop();
        if (!buffer) {
            buffer = new short[buffersize];
        }
        btype = tp;
    } else if (btype == PASSIVE) {
        if (buffer) {
            delete[] buffer;
            buffer = NULL;
        }
        btype = tp;
    }
}

part_init(PART_BUZZER_Name, cpart_Buzzer, "Output");
