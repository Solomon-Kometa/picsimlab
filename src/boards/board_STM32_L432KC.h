#ifndef BOARD_STM32_L432KC_H
#define BOARD_STM32_L432KC_H

#include <lxrad.h>
#include "bsim_qemu.h"

#define BOARD_STM32L432KC_NAME "STM32L432KC"

// new Board class
class cboard_STM32L432KC : public bsim_qemu {
private:
    unsigned char p_BUT;
    CLabel* label1;
    CCombo* combo1;
    void RegisterRemoteControl(void) override;

protected:
    const short int* GetPinMap(void) override;

public:
    // Constructor
    cboard_STM32L432KC(void);
    // Destructor
    ~cboard_STM32L432KC(void);
    // Return Board name
    lxString GetName(void) override { return lxT(BOARD_STM32L432KC_NAME); }
    lxString GetAboutInfo(void) override { return lxT("Solomon for now"); }

    // called every 100ms to draw board
    void Draw(CDraw* draw) override;

    void Run_CPU(void) override{};
    void Run_CPU_ns(uint64_t time) override;

    /*
    @return list of Board support Microcontroller
    @only "stm32f103rbt6" (M3) supported at the moment
    */
    lxString GetSupportedDevices(void) override { return lxT("stm32f103rbt6,"); }

    // reset board status
    void Reset(void) override;

    // Events on Board
    void EvMouseButtonPress(uint button, uint x, uint y, uint state) override;
    void EvMouseButtonRelease(uint button, uint x, uint y, uint state) override;
    void EvKeyPress(uint key, uint mask) override;
    void EvKeyRelease(uint key, uint mask) override;

    // Board Preferences
    // Save board preferences in configuration file
    void WritePreferences(void) override;
    // Access board from list of preferences
    void ReadPreferences(char* name, char* value) override;

    // Called every 1s to refresh status
    void RefreshStatus(void) override;

    // return input ids of names used in input map
    unsigned short GetInputId(char* name) override;

    // return output ids of names used in output map
    unsigned short GetOutputId(char* name) override;

    void board_Event(CControl* control) override;
    void MSetAPin(int pin, float value) override;

    lxString MGetPinName(int pin) override;

    int MGetPinCount(void) override;

    // void GetPinExtraConfig(int cfg) override;
};

#endif /*BOARD_STM32_L432KC_H*/
