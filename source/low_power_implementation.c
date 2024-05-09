/*
 * Copyright 2022-2023 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "low_power_implementation.h"
#include "fsl_cmc.h"
#include "fsl_spc.h"
#include "fsl_clock.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_lpuart.h"
#include "fsl_wuu.h"
#include "fsl_gpio.h"
#include "fsl_port.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define APP_POWER_MODE_NAME                                          \
    {                                                                \
        "Active", "Sleep", "DeepSleep", "PowerDown", "DeepPowerDown" \
    }

#define APP_WAKE_MODE_NAME                                          \
    {                                                               \
        "Typical wake up", "Fast wake up", "Slow wake up"           \
    }

#define APP_POWER_MODE_DESC                                                                                              			\
    {                                                                                                                    			\
        "Active: Core/System/Bus clock all ON.",                                                                         			\
        "Sleep: CPU clock is off, and the system clock and bus clock remain ON. ",                                       			\
        "Deep Sleep: Core/System/Bus clock are gated off. ",                                                             			\
        "Power Down: Core/System/Bus clock are gated off, CORE domain is in static state, Flash memory is powered off",  			\
        "Deep Power Down: The whole CORE domain is power gated."                                                         			\
    }

#define APP_SLEEP_WAKE_DESC                                                                                              			\
    {                                                                                                                    			\
        "Sleep Typical wake up time: ~0.27us, Power consumption: ~1.72mA",                                   						\
        "Sleep Fast wake up time: ~0.14us, Power consumption: ~3.27mA",                                      						\
        "Sleep Slow wake up time: ~1.04us, Power consumption: ~0.82mA"                                       						\
    }

#define APP_DeepSleep_WAKE_DESC                                                                                          			\
    {                                                                                                                    			\
        "DeepSleep Typical wake up time: ~7.52us(Production Sample), ~4.61us(Engineering Sample); Power consumption: ~22.1uA",   	\
        "DeepSleep Fast wake up time: ~5.90us(Production Sample), ~2.65us(Engineering Sample); Power consumption: ~965.2uA",        \
        "DeepSleep Slow wake up time: ~14.59us(Production Sample), ~11.98us(Engineering Sample); Power consumption: ~22.0uA"        \
    }

#define APP_PowerDown_WAKE_DESC                                                                                          			\
    {                                                                                                                    			\
        "PowerDown Typical wake up time: ~17.26us(Production Sample), ~13.99us(Engineering Sample); Power consumption: ~6.2uA",     \
        "PowerDown Fast wake up time: ~7.79us(Production Sample), ~4.49us(Engineering Sample); Power consumption: ~202.8uA",        \
        "PowerDown Slow wake up time: ~39.74us(Production Sample), ~36.89us(Engineering Sample); Power consumption: ~6.2uA"         \
    }

#define APP_DeepPowerDown_WAKE_DESC                                                                                      			\
    {                                                                                                                    			\
        "DeepPowerDown Typical wake up time: ~2.35ms(Production Sample), ~2.76ms(Engineering Sample); Power consumption: ~1.1uA"    \
    }

#define APP_CMC                         CMC

#define APP_SPC                         SPC0
#define APP_SPC_ISO_VALUE               (0x6U) /* VDD_USB, VDD_P2. */
#define APP_SPC_ISO_DOMAINS             "VDD_USB, VDD_P2"
#define APP_SPC_MAIN_POWER_DOMAIN       (kSPC_PowerDomain0)

#define APP_WUU                         WUU0
#define APP_WUU_WAKEUP_BUTTON_IDX       9U /* P1_7, SW3 on FRDM board. */
#define APP_WUU_WAKEUP_BUTTON_NAME      "SW3"

/* LPUART RX */
#define APP_DEBUG_CONSOLE_RX_PORT       PORT0
#define APP_DEBUG_CONSOLE_RX_GPIO       GPIO0
#define APP_DEBUG_CONSOLE_RX_PIN        2U
#define APP_DEBUG_CONSOLE_RX_PINMUX     kPORT_MuxAlt2
/* LPUART TX */
#define APP_DEBUG_CONSOLE_TX_PORT       PORT0
#define APP_DEBUG_CONSOLE_TX_GPIO       GPIO0
#define APP_DEBUG_CONSOLE_TX_PIN        3U
#define APP_DEBUG_CONSOLE_TX_PINMUX     kPORT_MuxAlt2

#define Lowpower_Test_GPIO              GPIO3
#define Lowpower_Test_GPIO_PIN          30U
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void APP_InitDebugConsole(void);
void APP_DeinitDebugConsole(void);
static void APP_SetSPCConfiguration(void);
static void APP_SetCMCConfiguration(void);

static void APP_SelectWakeupSource(void);
static void APP_GetWakeupConfig(app_power_mode_t targetMode);

static void APP_PowerPreSwitchHook(void);
static void APP_PowerPostSwitchHook(void);

static void APP_EnterSleepMode(void);
static void APP_EnterDeepSleepMode(void);
static void APP_EnterPowerDownMode(void);
static void APP_EnterDeepPowerDownMode(void);
static void APP_PowerModeSwitch(app_power_mode_t targetPowerMode);
static app_power_mode_t APP_GetTargetPowerMode(void);
static app_wakeup_mode_t APP_GetWakeUpMode(app_power_mode_t targetPowerMode);
static void APP_SetWakeUpMode(app_power_mode_t targetPowerMode, app_wakeup_mode_t targetWakeMode);
static void APP_SetSleepWakeUpMode(app_wakeup_mode_t targetWakeMode);
static void APP_SetDeepSleepWakeUpMode(app_wakeup_mode_t targetWakeMode);
static void APP_SetPowerDownWakeUpMode(app_wakeup_mode_t targetWakeMode);
static void APP_SetDeepPowerDownWakeUpMode(app_wakeup_mode_t targetWakeMode);

/*******************************************************************************
 * Variables
 ******************************************************************************/

char *const g_modeNameArray[] = APP_POWER_MODE_NAME;
char *const g_modeDescArray[] = APP_POWER_MODE_DESC;
char *const g_modeWakeArray[] = APP_WAKE_MODE_NAME;
char *const g_SleepWakeArray[] = APP_SLEEP_WAKE_DESC;
char *const g_DeepSleepWakeArray[] = APP_DeepSleep_WAKE_DESC;
char *const g_PowerDownWakeArray[] = APP_PowerDown_WAKE_DESC;
char *const g_DeepPowerDownWakeArray[] = APP_DeepPowerDown_WAKE_DESC;

/*******************************************************************************
 * Code
 ******************************************************************************/
void APP_InitDebugConsole(void)
{
    /*
     * Debug console RX pin is set to disable for current leakage, need to re-configure pinmux.
     * Debug console TX pin: Don't need to change.
     */
    BOARD_InitPins();
    BOARD_BootClockFRO48M(kSPC_CoreLDO_MidDriveVoltage, kSPC_CoreLDO_NormalDriveStrength, 
                          kSPC_CoreLDO_MidDriveVoltage, kSPC_CoreLDO_LowDriveStrength);
    BOARD_InitDebugConsole();
}

void APP_DeinitDebugConsole(void)
{
    DbgConsole_Deinit();
    PORT_SetPinMux(APP_DEBUG_CONSOLE_RX_PORT, APP_DEBUG_CONSOLE_RX_PIN, kPORT_PinDisabledOrAnalog);
    PORT_SetPinMux(APP_DEBUG_CONSOLE_TX_PORT, APP_DEBUG_CONSOLE_TX_PIN, kPORT_PinDisabledOrAnalog);
}

void main(void)
{
    uint32_t freq;
    app_power_mode_t targetPowerMode;
    app_wakeup_mode_t targetWakeMode;
    bool needSetWakeup = false;

    RESET_PeripheralReset(kLPUART0_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kPORT0_RST_SHIFT_RSTn);
    RESET_PeripheralReset(kGPIO3_RST_SHIFT_RSTn);

    BOARD_InitPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();
    
    /* Init GPIO for measure wake up time */
    gpio_pin_config_t gpio_config = {kGPIO_DigitalOutput, 0};
    GPIO_PinInit(Lowpower_Test_GPIO, Lowpower_Test_GPIO_PIN, &gpio_config);

    /* Release the I/O pads and certain peripherals to normal run mode state, for in Power Down mode
     * they will be in a latched state. */
    if ((CMC_GetSystemResetStatus(APP_CMC) & kCMC_WakeUpReset) != 0UL)
    {
        SPC_ClearPeriphIOIsolationFlag(APP_SPC);
    }

    APP_SetSPCConfiguration();
     
    /* clear wake up related flag for Deep Power Down */
    WUU0->PF|= WUU_PF_WUF9_MASK;                                                  
    NVIC_ClearPendingIRQ(WUU0_IRQn);                                              
    NVIC_ClearPendingIRQ(Reserved16_IRQn);                                               

    PRINTF("\r\nNormal Boot.\r\n");

    while (1)
    {
        GPIO_PortClear(Lowpower_Test_GPIO, 1 << Lowpower_Test_GPIO_PIN);
        if ((CMC_GetSystemResetStatus(APP_CMC) & kCMC_WakeUpReset) != 0UL)
        {
            /* Close ISO flags. */
            SPC_ClearPeriphIOIsolationFlag(APP_SPC);
        }

        /* Clear CORE_MAIN power domain's low power request flag. */
        SPC_ClearPowerDomainLowPowerRequestFlag(APP_SPC, APP_SPC_MAIN_POWER_DOMAIN);
        SPC_ClearLowPowerRequest(APP_SPC);

        /* Normal start. */
        APP_SetCMCConfiguration();

        freq = CLOCK_GetFreq(kCLOCK_CoreSysClk);
        PRINTF("\r\n###########################    Low Power Implementation Demo    ###########################\r\n");
        PRINTF("    Core Clock = %dHz \r\n", freq);
        PRINTF("    Power mode: Active\r\n");
        targetPowerMode = APP_GetTargetPowerMode();

        if ((targetPowerMode > kAPP_PowerModeMin) && (targetPowerMode < kAPP_PowerModeMax))
        {
            /* If target mode is Active mode, don't need to set wakeup source. */
            if (targetPowerMode == kAPP_PowerModeActive)
            {
                needSetWakeup = false;
            }
            else
            {
                needSetWakeup = true;
            }
        }

        /* Print description of selected power mode. */
        if (needSetWakeup)
        {       
            /* select the wake up mode */
            targetWakeMode = APP_GetWakeUpMode(targetPowerMode);
            /* configure the wake up mode*/
            APP_SetWakeUpMode(targetPowerMode, targetWakeMode);
            /* select and configure the wake up source */
            APP_GetWakeupConfig(targetPowerMode);
            APP_PowerPreSwitchHook();
            /* enter different low power mode */
            APP_PowerModeSwitch(targetPowerMode);
            APP_PowerPostSwitchHook();
        }

        PRINTF("\r\nNext loop.\r\n");
    }
}

static void APP_SetSPCConfiguration(void)
{
    status_t status;

    spc_active_mode_regulators_config_t activeModeRegulatorOption;

    SPC_EnableSRAMLdo(APP_SPC, true);
    
    /* Disable all modules that controlled by SPC in active mode.. */
    SPC_DisableActiveModeAnalogModules(APP_SPC, kSPC_controlAllModules);

    /* Disable LVDs and HVDs */
    SPC_EnableActiveModeCoreLowVoltageDetect(APP_SPC, false);
    SPC_EnableActiveModeSystemHighVoltageDetect(APP_SPC, false);
    SPC_EnableActiveModeSystemLowVoltageDetect(APP_SPC, false);

    activeModeRegulatorOption.bandgapMode = kSPC_BandgapEnabledBufferDisabled;
    activeModeRegulatorOption.CoreLDOOption.CoreLDOVoltage       = kSPC_CoreLDO_MidDriveVoltage;
    activeModeRegulatorOption.CoreLDOOption.CoreLDODriveStrength = kSPC_CoreLDO_NormalDriveStrength;

    status = SPC_SetActiveModeRegulatorsConfig(APP_SPC, &activeModeRegulatorOption);
    /* Disable Vdd Core Glitch detector in active mode. */
    SPC_DisableActiveModeVddCoreGlitchDetect(APP_SPC, true);
    if (status != kStatus_Success)
    {
        PRINTF("Fail to set regulators in Active mode.");
        return;
    }
    while (SPC_GetBusyStatusFlag(APP_SPC))
        ;

    SPC_DisableLowPowerModeAnalogModules(APP_SPC, kSPC_controlAllModules);
    SPC_SetLowPowerWakeUpDelay(APP_SPC, 0xFF);
    spc_lowpower_mode_regulators_config_t lowPowerRegulatorOption;

    lowPowerRegulatorOption.lpIREF      = false;
    lowPowerRegulatorOption.bandgapMode = kSPC_BandgapDisabled;
    lowPowerRegulatorOption.CoreLDOOption.CoreLDOVoltage       = kSPC_CoreLDO_MidDriveVoltage;
    lowPowerRegulatorOption.CoreLDOOption.CoreLDODriveStrength = kSPC_CoreLDO_LowDriveStrength;

    status = SPC_SetLowPowerModeRegulatorsConfig(APP_SPC, &lowPowerRegulatorOption);
    /* Disable Vdd Core Glitch detector in low power mode. */
    SPC_DisableLowPowerModeVddCoreGlitchDetect(APP_SPC, true);
    if (status != kStatus_Success)
    {
        PRINTF("Fail to set regulators in Low Power Mode.");
        return;
    }
    while (SPC_GetBusyStatusFlag(APP_SPC))
        ;
}

static void APP_SetCMCConfiguration(void)
{
    /* Disable low power debug. */
    CMC_EnableDebugOperation(APP_CMC, false);
    /* Allow all power mode */
    CMC_SetPowerModeProtection(APP_CMC, kCMC_AllowAllLowPowerModes);

    /* Disable flash memory accesses and place flash memory in low-power state whenever the core clock
       is gated. And an attempt to access the flash memory will cause the flash memory to exit low-power
       state for the duration of the flash memory access. */
    CMC_ConfigFlashMode(APP_CMC, false, true, false);
}

static app_power_mode_t APP_GetTargetPowerMode(void)
{
    uint8_t ch;

    app_power_mode_t inputPowerMode;

    do
    {
        PRINTF("\r\nSelect the desired operation \n\r\n");
        for (app_power_mode_t modeIndex = kAPP_PowerModeActive; modeIndex <= kAPP_PowerModeDeepPowerDown; modeIndex++)
        {
            PRINTF("\tPress %c to enter: %s mode\r\n", modeIndex,
                   g_modeNameArray[(uint8_t)(modeIndex - kAPP_PowerModeActive)]);
        }

        PRINTF("\r\nWaiting for power mode select...\r\n\r\n");

        ch = GETCHAR();

        if ((ch >= 'a') && (ch <= 'z'))
        {
            ch -= 'a' - 'A';
        }
        inputPowerMode = (app_power_mode_t)ch;

        if ((inputPowerMode > kAPP_PowerModeDeepPowerDown) || (inputPowerMode < kAPP_PowerModeActive))
        {
            PRINTF("Wrong Input!");
        }
    } while (inputPowerMode > kAPP_PowerModeDeepPowerDown);

    PRINTF("\tPress %c and select %s mode\r\n", inputPowerMode, g_modeNameArray[(uint8_t)(inputPowerMode - kAPP_PowerModeActive)]);
    PRINTF("\t%s\r\n", g_modeDescArray[(uint8_t)(inputPowerMode - kAPP_PowerModeActive)]);

    return inputPowerMode;
}

static app_wakeup_mode_t APP_GetWakeUpMode(app_power_mode_t targetPowerMode)
{
    uint8_t ch;

    app_wakeup_mode_t wakeUpMode;
    
    if(targetPowerMode < kAPP_PowerModeDeepPowerDown)
    {
        do
        {
            PRINTF("\r\nSelect the wake up mode \n\r\n");

            for (app_wakeup_mode_t modeIndex = kAPP_TypicalWakeUp; modeIndex <= kAPP_SlowWakeUp; modeIndex++)
            {
                PRINTF("\tPress %c to select: %s mode\r\n", modeIndex,
                       g_modeWakeArray[(uint8_t)(modeIndex - kAPP_TypicalWakeUp)]);
            }

            PRINTF("\r\nWaiting for wake up mode select...\r\n\r\n");

            ch = GETCHAR();
            wakeUpMode = (app_wakeup_mode_t)ch;
            
            if ((wakeUpMode > kAPP_SlowWakeUp) || (wakeUpMode < kAPP_TypicalWakeUp))
            {
                PRINTF("Wrong Input!");
            }
        } while (wakeUpMode > kAPP_SlowWakeUp || wakeUpMode < kAPP_TypicalWakeUp);
    }
    else
    {
        do
        {
            PRINTF("\r\nSelect the wake up mode \n\r\n");

            for (app_wakeup_mode_t modeIndex = kAPP_TypicalWakeUp; modeIndex <= kAPP_TypicalWakeUp; modeIndex++)
            {
                PRINTF("\tPress %c to select: %s mode\r\n", modeIndex,
                       g_modeWakeArray[(uint8_t)(modeIndex - kAPP_TypicalWakeUp)]);
            }

            PRINTF("\r\nWaiting for wake up mode select...\r\n\r\n");

            ch = GETCHAR();
            wakeUpMode = (app_wakeup_mode_t)ch;
            
            if (wakeUpMode != kAPP_TypicalWakeUp)
            {
                PRINTF("Wrong Input!");
            }
        } while (wakeUpMode != kAPP_TypicalWakeUp);
    }
    PRINTF("\tPress %c and select %s mode\r\n", wakeUpMode, g_modeWakeArray[(uint8_t)(wakeUpMode - kAPP_TypicalWakeUp)]);
    switch (targetPowerMode)
    {
        case kAPP_PowerModeSleep:
            PRINTF("\t%s\n\r\n", g_SleepWakeArray[(uint8_t)(wakeUpMode - kAPP_TypicalWakeUp)]);
            break;
        case kAPP_PowerModeDeepSleep:
            PRINTF("\t%s\n\r\n", g_DeepSleepWakeArray[(uint8_t)(wakeUpMode - kAPP_TypicalWakeUp)]);
            break;
        case kAPP_PowerModePowerDown:
            PRINTF("\t%s\n\r\n", g_PowerDownWakeArray[(uint8_t)(wakeUpMode - kAPP_TypicalWakeUp)]);
            break;
        case kAPP_PowerModeDeepPowerDown:
            PRINTF("\t%s\n\r\n", g_DeepPowerDownWakeArray[(uint8_t)(wakeUpMode - kAPP_TypicalWakeUp)]);
            break;
        default:
            assert(false);
            break;
    }
    return wakeUpMode;
}

static void APP_SetWakeUpMode(app_power_mode_t targetPowerMode, app_wakeup_mode_t targetWakeMode)
{
    if (targetPowerMode != kAPP_PowerModeActive)
    {
        switch (targetPowerMode)
        {
            case kAPP_PowerModeSleep:
                APP_SetSleepWakeUpMode(targetWakeMode);
                break;
            case kAPP_PowerModeDeepSleep:
                APP_SetDeepSleepWakeUpMode(targetWakeMode);
                break;
            case kAPP_PowerModePowerDown:
                APP_SetPowerDownWakeUpMode(targetWakeMode);
                break;
            case kAPP_PowerModeDeepPowerDown:
              APP_SetDeepPowerDownWakeUpMode(targetWakeMode);
                break;
            default:
                assert(false);
                break;
        }
    }
}

static void APP_SetSleepWakeUpMode(app_wakeup_mode_t targetWakeMode)
{
      switch (targetWakeMode)
      {
          case kAPP_TypicalWakeUp:
              /* the least wake up delay */
              SPC0->LPWKUP_DELAY &= ~SPC_LPWKUP_DELAY_LPWKUP_DELAY_MASK;
              SPC0->LPWKUP_DELAY |= SPC_LPWKUP_DELAY_LPWKUP_DELAY(0x00);
              BOARD_BootClockFRO48M(kSPC_CoreLDO_MidDriveVoltage, kSPC_CoreLDO_NormalDriveStrength, 
                                    kSPC_CoreLDO_MidDriveVoltage, kSPC_CoreLDO_LowDriveStrength);
              break;
          case kAPP_FastWakeUp:
              /* the least wake up delay */
              SPC0->LPWKUP_DELAY &= ~SPC_LPWKUP_DELAY_LPWKUP_DELAY_MASK;
              SPC0->LPWKUP_DELAY |= SPC_LPWKUP_DELAY_LPWKUP_DELAY(0x00);
              BOARD_BootClockFRO96M(kSPC_CoreLDO_NormalVoltage, kSPC_CoreLDO_NormalDriveStrength, 
                                    kSPC_CoreLDO_MidDriveVoltage, kSPC_CoreLDO_LowDriveStrength);
              break;
          case kAPP_SlowWakeUp:
              /* the least wake up delay */
              SPC0->LPWKUP_DELAY &= ~SPC_LPWKUP_DELAY_LPWKUP_DELAY_MASK;
              SPC0->LPWKUP_DELAY |= SPC_LPWKUP_DELAY_LPWKUP_DELAY(0x00);
              BOARD_BootClockFRO12M(kSPC_CoreLDO_MidDriveVoltage, kSPC_CoreLDO_LowDriveStrength, 
                                    kSPC_CoreLDO_MidDriveVoltage, kSPC_CoreLDO_LowDriveStrength);
              break;
          default:
              assert(false);
              break;
      }
}

static void APP_SetDeepSleepWakeUpMode(app_wakeup_mode_t targetWakeMode)
{
      switch (targetWakeMode)
      {
          case kAPP_TypicalWakeUp:
              /* the least wake up delay */
              SPC0->LPWKUP_DELAY &= ~SPC_LPWKUP_DELAY_LPWKUP_DELAY_MASK;
              SPC0->LPWKUP_DELAY |= SPC_LPWKUP_DELAY_LPWKUP_DELAY(0x00);
              BOARD_BootClockFRO48M(kSPC_CoreLDO_MidDriveVoltage, kSPC_CoreLDO_NormalDriveStrength, 
                                    kSPC_CoreLDO_MidDriveVoltage, kSPC_CoreLDO_LowDriveStrength);
              break;
          case kAPP_FastWakeUp:
              /* the least wake up delay */
              SPC0->LPWKUP_DELAY &= ~SPC_LPWKUP_DELAY_LPWKUP_DELAY_MASK;
              SPC0->LPWKUP_DELAY |= SPC_LPWKUP_DELAY_LPWKUP_DELAY(0x00);
              /* enable FIRC and SIRC in DeepSleep mode for fast wake up */
              SCG0->FIRCCSR &= ~SCG_FIRCCSR_LK_MASK;
              SCG0->SIRCCSR &= ~SCG_SIRCCSR_LK_MASK;
              SCG0->FIRCCSR |= SCG_FIRCCSR_FIRCSTEN_MASK;
              SCG0->SIRCCSR |= SCG_SIRCCSR_SIRCSTEN_MASK;
              SCG0->FIRCCSR |= SCG_FIRCCSR_LK_MASK;
              SCG0->SIRCCSR |= SCG_SIRCCSR_LK_MASK;
              BOARD_BootClockFRO96M(kSPC_CoreLDO_NormalVoltage, kSPC_CoreLDO_NormalDriveStrength, 
                                    kSPC_CoreLDO_NormalVoltage, kSPC_CoreLDO_NormalDriveStrength);
              break;
          case kAPP_SlowWakeUp:
              /* the least wake up delay */
              SPC0->LPWKUP_DELAY &= ~SPC_LPWKUP_DELAY_LPWKUP_DELAY_MASK;
              SPC0->LPWKUP_DELAY |= SPC_LPWKUP_DELAY_LPWKUP_DELAY(0x00);
              BOARD_BootClockFRO12M(kSPC_CoreLDO_MidDriveVoltage, kSPC_CoreLDO_LowDriveStrength, 
                                    kSPC_CoreLDO_MidDriveVoltage, kSPC_CoreLDO_LowDriveStrength);
              break;
          default:
              assert(false);
              break;
      }
}

static void APP_SetPowerDownWakeUpMode(app_wakeup_mode_t targetWakeMode)
{
      switch (targetWakeMode)
      {
          case kAPP_TypicalWakeUp:
              /* Wake up delay for LDO recovery */
              SPC0->LPWKUP_DELAY &= ~SPC_LPWKUP_DELAY_LPWKUP_DELAY_MASK;
              SPC0->LPWKUP_DELAY |= SPC_LPWKUP_DELAY_LPWKUP_DELAY(0x5B);
              BOARD_BootClockFRO48M(kSPC_CoreLDO_MidDriveVoltage, kSPC_CoreLDO_NormalDriveStrength, 
                                    kSPC_CoreLDO_UnderDriveVoltage, kSPC_CoreLDO_LowDriveStrength);
              break;
          case kAPP_FastWakeUp:
              /* the least wake up delay */
              SPC0->LPWKUP_DELAY &= ~SPC_LPWKUP_DELAY_LPWKUP_DELAY_MASK;
              SPC0->LPWKUP_DELAY |= SPC_LPWKUP_DELAY_LPWKUP_DELAY(0x00);
              BOARD_BootClockFRO96M(kSPC_CoreLDO_NormalVoltage, kSPC_CoreLDO_NormalDriveStrength, 
                                    kSPC_CoreLDO_NormalVoltage, kSPC_CoreLDO_NormalDriveStrength);
              break;
          case kAPP_SlowWakeUp:
              /* the longest wake up delay */
              SPC0->LPWKUP_DELAY &= ~SPC_LPWKUP_DELAY_LPWKUP_DELAY_MASK;
              SPC0->LPWKUP_DELAY |= SPC_LPWKUP_DELAY_LPWKUP_DELAY(0xFF);
              BOARD_BootClockFRO12M(kSPC_CoreLDO_MidDriveVoltage, kSPC_CoreLDO_NormalDriveStrength, 
                                    kSPC_CoreLDO_UnderDriveVoltage, kSPC_CoreLDO_LowDriveStrength);
              break;
          default:
              assert(false);
              break;
      }
}

static void APP_SetDeepPowerDownWakeUpMode(app_wakeup_mode_t targetWakeMode)
{
    switch (targetWakeMode)
    {
        case kAPP_TypicalWakeUp:
        	/* the least wake up delay */
            SPC0->LPWKUP_DELAY &= ~SPC_LPWKUP_DELAY_LPWKUP_DELAY_MASK;
            SPC0->LPWKUP_DELAY |= SPC_LPWKUP_DELAY_LPWKUP_DELAY(0x00);
            BOARD_BootClockFRO48M(kSPC_CoreLDO_MidDriveVoltage, kSPC_CoreLDO_NormalDriveStrength,
                                  kSPC_CoreLDO_MidDriveVoltage, kSPC_CoreLDO_LowDriveStrength);
            break;
        default:
            assert(false);
            break;
    }
}

static void APP_GetWakeupConfig(app_power_mode_t targetMode)
{
//    char *isoDomains = NULL;

    APP_SelectWakeupSource();

    if (targetMode > kAPP_PowerModeSleep)
    {
        /* Isolate some power domains that are not used in low power modes.*/
        SPC_SetExternalVoltageDomainsConfig(APP_SPC, APP_SPC_ISO_VALUE, 0x0U);
//        isoDomains = APP_SPC_ISO_DOMAINS;
//        PRINTF("Isolate power domains: %s\r\n", isoDomains);
    }
}

static void APP_SelectWakeupSource(void)
{
      PRINTF("Wakeup Button Selected As Wakeup Source.\r\n");
      /* Set WUU to detect on rising edge for all power modes. */
      wuu_external_wakeup_pin_config_t wakeupButtonConfig;

      wakeupButtonConfig.edge  = kWUU_ExternalPinFallingEdge;
      wakeupButtonConfig.event = kWUU_ExternalPinInterrupt;
      wakeupButtonConfig.mode  = kWUU_ExternalPinActiveAlways;
      WUU_SetExternalWakeUpPinsConfig(APP_WUU, APP_WUU_WAKEUP_BUTTON_IDX, &wakeupButtonConfig);
      PRINTF("Entering Low power mode...\r\n");
      PRINTF("Please press %s to wakeup.(Please only press the wakeup button when this message appears, otherwise it will result in failure to wake up!)\r\n", APP_WUU_WAKEUP_BUTTON_NAME);
}

static void APP_PowerPreSwitchHook(void)
{
    /* Wait for debug console output finished. */
    while (!(kLPUART_TransmissionCompleteFlag & LPUART_GetStatusFlags((LPUART_Type *)BOARD_DEBUG_UART_BASEADDR)))
    {
    }
    APP_DeinitDebugConsole();
}

static void APP_PowerPostSwitchHook(void)
{
    BOARD_BootClockFRO48M(kSPC_CoreLDO_MidDriveVoltage, kSPC_CoreLDO_NormalDriveStrength, 
                          kSPC_CoreLDO_MidDriveVoltage, kSPC_CoreLDO_LowDriveStrength);
    APP_InitDebugConsole();
}

static void APP_PowerModeSwitch(app_power_mode_t targetPowerMode)
{
    if (targetPowerMode != kAPP_PowerModeActive)
    {
        switch (targetPowerMode)
        {
            case kAPP_PowerModeSleep:
                APP_EnterSleepMode();
                break;
            case kAPP_PowerModeDeepSleep:
                APP_EnterDeepSleepMode();
                break;
            case kAPP_PowerModePowerDown:
                APP_EnterPowerDownMode();
                break;
            case kAPP_PowerModeDeepPowerDown:
                APP_EnterDeepPowerDownMode();
                break;
            default:
                assert(false);
                break;
        }
    }
}

static void APP_EnterSleepMode(void)
{
    cmc_power_domain_config_t config;

    config.clock_mode  = kCMC_GateNoneClock;
    config.main_domain = kCMC_ActiveOrSleepMode;

    CMC_EnterLowPowerMode(APP_CMC, &config);
}

static void APP_EnterDeepSleepMode(void)
{
    cmc_power_domain_config_t config;

    config.clock_mode  = kCMC_GateAllSystemClocksEnterLowPowerMode;
    config.main_domain = kCMC_DeepSleepMode;

    CMC_EnterLowPowerMode(APP_CMC, &config);
    
    SCG0->FIRCCSR &= ~SCG_FIRCCSR_LK_MASK;
    SCG0->SIRCCSR &= ~SCG_SIRCCSR_LK_MASK;
    SCG0->FIRCCSR &= ~SCG_FIRCCSR_FIRCSTEN_MASK;
    SCG0->SIRCCSR &= ~SCG_SIRCCSR_SIRCSTEN_MASK;
    SCG0->FIRCCSR |= SCG_FIRCCSR_LK_MASK;
    SCG0->SIRCCSR |= SCG_SIRCCSR_LK_MASK;
}

static void APP_EnterPowerDownMode(void)
{
    cmc_power_domain_config_t config;

    config.clock_mode  = kCMC_GateAllSystemClocksEnterLowPowerMode;
    config.main_domain = kCMC_PowerDownMode;

    CMC_EnterLowPowerMode(APP_CMC, &config);
}

static void APP_EnterDeepPowerDownMode(void)
{
    cmc_power_domain_config_t config;

    config.clock_mode  = kCMC_GateAllSystemClocksEnterLowPowerMode;
    config.main_domain = kCMC_DeepPowerDown;

    CMC_EnterLowPowerMode(APP_CMC, &config);
}
