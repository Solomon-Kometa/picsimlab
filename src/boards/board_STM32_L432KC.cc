#include "board_STM32_L432KC.h"
#include "../lib/oscilloscope.h"
#include "../lib/picsimlab.h"
#include "../lib/spareparts.h"

// input ids from input map
enum {
    I_USB,  // usb connector
    I_PWR,  // Jumper for power
    I_RST,  // Reset Buttom
};

// Output ids from output map
enum {
    O_RGB,   // CommuniCboard_cation LED
    O_LED2,  // Power LED
    O_LED3   // Program LED(green)
};

#define A 0x1000
#define B 0x2000
#define C 0x3000
#define H 0x4000

static const short int pinmap[33] = {
    32,      // number of pins
    -1,      // 1 VDD
    C | 14,  // 2 C14
    C | 15,  // 3 C15
    -1,      // 4 NRST
    -1,      // 5 VDD
    A | 0,   // 6 A0
    A | 1,   // 7 A1
    A | 2,   // 8 A2
    A | 3,   // 9 A3
    A | 4,   // 10 A4
    A | 5,   // 11 A5
    A | 6,   // 12 A6
    A | 7,   // 13 A7
    B | 0,   // 14 B0
    B | 1,   // 15 B1
    -1,      // 16 VSS
    -1,      // 17 VDD
    A | 8,   // 18 A8
    A | 9,   // 19 A9
    A | 10,  // 20 A10
    A | 11,  // 21 A11
    A | 12,  // 22 A12
    A | 13,  // 23 A13
    A | 14,  // 24 A14
    A | 15,  // 25 A15
    B | 3,   // 26 B3
    B | 4,   // 27 B4
    B | 5,   // 28 B5
    B | 6,   // 29 B6
    B | 7,   // 30 B7
    H | 3,   // 31 H3
    -1,      // 32 VSS

};

const short int* cboard_STM32L432KC::GetPinMap() {
    return pinmap;
}

// return input ids from input map
unsigned short cboard_STM32L432KC::GetInputId(char* name) {
    if (strcmp(name, "PG_USB") == 0)
        return I_USB;
    if (strcmp(name, "JP_PWR") == 0)
        return I_PWR;
    if (strcmp(name, "PB_RST") == 0)
        return I_RST;
    else {
        printf("Error: Input %s has no valid ID!\n", name);
        return -1;
    }
}

// return output ids from output map
unsigned short cboard_STM32L432KC::GetOutputId(char* name) {
    if (strcmp(name, "LR_RGB") == 0)
        return O_RGB;
    else if (strcmp(name, "LD_LED2") == 0)
        return O_LED2;
    else if (strcmp(name, "LD_LED3") == 0)
        return O_LED3;
    else {
        printf("Error: %s has no id in Map!\n", name);
        return -1;
    }
}

cboard_STM32L432KC::cboard_STM32L432KC(void) {
    char buffer[1024];

    SimType = QEMU_SIM_STM32;
    Proc = "stm32f103rbt6";  // default Micfŕocontroller in PICSimLab

    // Read board map(input and output)
    ReadMaps();
    p_BUT = 0;

    if (PICSimLab.GetWindow()) {
        // label1
        label1 = new CLabel();
        label1->SetFOwner(PICSimLab.GetWindow());
        label1->SetName(lxT("label1_"));
        label1->SetX(13);
        label1->SetY(54 + 20);
        label1->SetWidth(120);
        label1->SetHeight(24);
        label1->SetEnable(1);
        label1->SetVisible(1);
        label1->SetText(lxT("Qemu CPU MIPS"));
        label1->SetAlign(1);
        PICSimLab.GetWindow()->CreateChild(label1);
        // combo1
        combo1 = new CCombo();
        combo1->SetFOwner(PICSimLab.GetWindow());
        combo1->SetName(lxT("combo1_"));
        combo1->SetX(13);
        combo1->SetY(78 + 20);
        combo1->SetWidth(130);
        combo1->SetHeight(24);
        combo1->SetEnable(1);
        combo1->SetVisible(1);
        combo1->SetText(IcountToMipsStr(icount));
        combo1->SetItems(IcountToMipsItens(buffer));
        combo1->SetTag(3);
        combo1->EvOnComboChange = PICSimLab.board_Event;
        PICSimLab.GetWindow()->CreateChild(combo1);
    }
}

cboard_STM32L432KC::~cboard_STM32L432KC(void) {
    if (PICSimLab.GetWindow()) {
        PICSimLab.GetWindow()->DestroyChild(label1);
        PICSimLab.GetWindow()->DestroyChild(combo1);
    }
}

// Reset Board status
void cboard_STM32L432KC::Reset(void) {
    p_BUT = 0;
    MReset(1);

    if (PICSimLab.GetStatusBar()) {
        PICSimLab.GetStatusBar()->SetField(2, lxT("Serial: ") + lxString::FromAscii(SERIALDEVICE));
    }

    if (use_spare)
        SpareParts.Reset();

    RegisterRemoteControl();
}

void cboard_STM32L432KC::WritePreferences(void) {
    // Write board Microcontroller to preferences
    PICSimLab.SavePrefs(lxT("STM32L432KC_proc"), Proc);
    // Write microcontroller clock to prferences
    PICSimLab.SavePrefs(lxT("STM32L432KC_clock"), lxString().Format("%2.1f", PICSimLab.GetClock()));
    // Write Microcontroller icount to preferences
    PICSimLab.SavePrefs(lxT("STM32L432KC_icount"), itoa(icount));
}

void cboard_STM32L432KC::ReadPreferences(char* name, char* value) {
    // Read microcontroller
    if (!strcmp(name, "STM32L432KC_proc"))
        Proc = value;
    // Read microcontroller clock
    if (!strcmp(name, "STM32L432KC_clock"))
        PICSimLab.SetClock(atof(value));
    // read microcontroller icount
    if (!strcmp(name, "STM32L432KC_icount")) {
        icount = atoi(value);
        if (combo1) {
            combo1->SetText(IcountToMipsStr(icount));
        }
    }
}

void cboard_STM32L432KC::RegisterRemoteControl(void) {
    output_ids[O_LED3]->status = &pins[26].oavalue;
}

void cboard_STM32L432KC::RefreshStatus(void) {
    if (serial_open) {
        PICSimLab.GetStatusBar()->SetField(2, lxT("Serial: ") + lxString::FromAscii(SERIALDEVICE));
    } else {
        PICSimLab.GetStatusBar()->SetField(2, lxT("Serial: Error"));
    }
}

// Designate inpout and output areas
void cboard_STM32L432KC::Draw(CDraw* draw) {
    int i;

    draw->Canvas.Init(Scale, Scale);  // initialize draw context
    // run over all outputs
    for (i = 0; i < outputc; i++) {
        // check if output is a rectangle
        if (!output[i].r) {
            draw->Canvas.SetFgColor(255, 255, 255);
            switch (output[i].id) {
                case O_LED3:
                    draw->Canvas.SetColor(0, pins[26].oavalue, 0);
                    draw->Canvas.Rectangle(1, output[i].x1, output[i].y1, (output[i].x2 - output[i].x1),
                                           (output[i].y2 - output[i].y1));
                    break;

                case O_LED2:
                    draw->Canvas.SetColor(200 * PICSimLab.GetMcuPwr() + 55, 0, 0);
                    draw->Canvas.Rectangle(1, output[i].x1, output[i].y1, (output[i].x2 - output[i].x1),
                                           (output[i].y2 - output[i].y1));
                    break;
            }
        }
    }
    draw->Canvas.End();
    draw->Update();
}

void cboard_STM32L432KC::Run_CPU_ns(uint64_t time) {
    static int j = 0;
    static unsigned char pi = 0;
    static unsigned int alm[64];  // to be clearified
    static const int pinc = MGetPinCount();

    const int JUMPSTEPS = 4.0 * PICSimLab.GetJUMPSTEPS();  // number of steps skipped in Board update

    // const int inc = 1000000000L / MGetInstClockFreq();

    const float RNSTEPS = 200.0 * pinc * inc_ns / TTIMEOUT;

    for (uint32_t c = 0; c < time; c += inc_ns) {
        if (ns_count < inc_ns) {
            memset(alm, 0, 64 * sizeof(unsigned int));

            // Spare parts window pre process
            if (use_spare)
                SpareParts.PreProcess();

            j = JUMPSTEPS;  // step counter
            pi = 0;
        }

        if (PICSimLab.GetMcuPwr())  // if powered
        {
            MStep();
            InstCounterInc();

            // Oscilloscope window process
            if (use_oscope)
                Oscilloscope.SetSample();
            // Spare parts window process
            if (use_spare)
                SpareParts.Process();

            // inceament mean value counter if pin is high
            alm[pi] += pins[pi].value;
            pi++;
            if (pi == pinc)
                pi = 0;

            if (j >= JUMPSTEPS)
                j = -1;  // reset counter

            j++;
        }

        ns_count += inc_ns;
        if (ns_count >= TTIMEOUT) {
            ns_count -= TTIMEOUT;
            // calculate mean value
            for (pi = 0; pi < MGetPinCount(); pi++) {
                pins[pi].oavalue = (int)((alm[pi] * RNSTEPS) + 55);
            }
        }
    }
}

void cboard_STM32L432KC::EvMouseButtonPress(uint button, uint x, uint y, uint state) {
    int i;

    // search for the input area which owner the event
    for (i = 0; i < inputc; i++) {
        if (((input[i].x1 <= x) && (input[i].x2 >= x)) && ((input[i].y1 <= y) && (input[i].y2 >= y))) {
            switch (input[i].id) {
                    // if event is over I_ISCP area then load hex file
                case I_USB:
                    PICSimLab.OpenLoadHexFileDialog();
                    ;
                    break;
                    // if event is over I_PWR area then toggle board on/off
                case I_PWR:
                    if (PICSimLab.GetMcuPwr())  // if on turn off
                    {
                        PICSimLab.SetMcuPwr(0);
                        Reset();
                    } else  // if off turn on
                    {
                        PICSimLab.SetMcuPwr(1);
                        Reset();
                    }
                    break;
                    // if event is over I_RST area then turn off and reset
                case I_RST:
                    /*
                    if (PICSimLab.GetWindow()->Get_mcupwr () && reset (-1))//if powered
                     {
                      PICSimLab.Set_mcupwr (0);
                      PICSimLab.Set_mcurst (1);
                     }
                     */
                    Reset();
                    p_RST = 0;
                    break;
            }
        }
    }
}

void cboard_STM32L432KC::EvMouseButtonRelease(uint button, uint x, uint y, uint state) {
    int i;

    // search for the input area which owner the event
    for (i = 0; i < inputc; i++) {
        if (((input[i].x1 <= x) && (input[i].x2 >= x)) && ((input[i].y1 <= y) && (input[i].y2 >= y))) {
            switch (input[i].id) {
                    // if event is over I_RST area then turn on
                case I_RST:
                    if (PICSimLab.GetMcuRst())  // if powered
                    {
                        PICSimLab.SetMcuPwr(1);
                        PICSimLab.SetMcuRst(0);
                        /*
                                 if (reset (-1))
                                  {
                                   Reset ();
                                  }
                         */
                    }
                    p_RST = 1;
                    break;
            }
        }
    }
}

void cboard_STM32L432KC::EvKeyPress(uint key, uint mask) {
    printf("Still to be implemented");
}

void cboard_STM32L432KC::EvKeyRelease(uint key, uint mask) {
    printf("Still to be implemented");
}

void cboard_STM32L432KC::board_Event(CControl* control) {
    icount = MipsStrToIcount(combo1->GetText().c_str());
    PICSimLab.EndSimulation();
}

void cboard_STM32L432KC::MSetAPin(int pin, float value) {
    printf("To be defined");
}

lxString cboard_STM32L432KC::MGetPinName(int pin) {
    lxString pinname = "Error";
    switch (pin) {
        case 1:
            pinname = "VDD";
            break;
        case 2:
            pinname = "PC14";
            break;
        case 3:
            pinname = "PC15";
            break;
        case 4:
            pinname = "NRST";
            break;
        case 5:
            pinname = "VDD";
            break;
        case 6:
            pinname = "PA0";
            break;
        case 7:
            pinname = "PA1";
            break;
        case 8:
            pinname = "PA2";
            break;
        case 9:
            pinname = "PA3";
            break;
        case 10:
            pinname = "PA4";
            break;
        case 11:
            pinname = "PA5";
            break;
        case 12:
            pinname = "PA6";
            break;
        case 13:
            pinname = "PA7";
            break;
        case 14:
            pinname = "PB0";
            break;
        case 15:
            pinname = "PB1";
            break;
        case 16:
            pinname = "VSS";
            break;
        case 17:
            pinname = "VDD";
            break;
        case 18:
            pinname = "PA8";
            break;
        case 19:
            pinname = "PA9";
            break;
        case 20:
            pinname = "PA10";
            break;
        case 21:
            pinname = "PA11";
            break;
        case 22:
            pinname = "PA12";
            break;
        case 23:
            pinname = "PA13";
            break;
        case 24:
            pinname = "PA14";
            break;
        case 25:
            pinname = "PA15";
            break;
        case 26:
            pinname = "PB3";
            break;
        case 27:
            pinname = "PB4";
            break;
        case 28:
            pinname = "PB5";
            break;
        case 29:
            pinname = "PB6";
            break;
        case 30:
            pinname = "PB7";
            break;
        case 31:
            pinname = "PH3-BOOT0";
            break;
        case 32:
            pinname = "VSS";
            break;
    }
    return pinname;
}

int cboard_STM32L432KC::MGetPinCount(void) {
    return 32;
}

// void cboard_STM32L432KC::GetPinExtraConfig(int cfg) {}

// Register Board in PICSimLab
board_init(BOARD_STM32L432KC_NAME, cboard_STM32L432KC);
