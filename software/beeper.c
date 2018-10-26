#include "beeper.h"
#include "settings.h"
#include "inc/stm8s_beep.h"
#include "inc/stm8s_flash.h"

void beeper_init()
{
    // Unlock flash
    //TODO: Is unlocked flash required elsewhere as well?
    FLASH->DUKR = FLASH_RASS_KEY2;
    FLASH->DUKR = FLASH_RASS_KEY1;
    while (!(FLASH->IAPSR & FLASH_IAPSR_DUL));

    // Set option bytes because of buzzer, not working yet
    /*if(OPT->OPT2 != 0x80 || OPT->OPT3 != 0x08){
        FLASH->CR2 |= FLASH_CR2_OPT; // unlock option bytes for writing
        FLASH->NCR2 &= (uint8_t)(~FLASH_NCR2_NOPT);
        while (!(FLASH->IAPSR & FLASH_IAPSR_DUL));

        OPT->OPT2 = 0x80; // enable LSI clock source
        OPT->NOPT2 = 0x7f;
        OPT->OPT3 = 0x08; // set PD4 as alternative buzzer output
        OPT->NOPT3 = 0xf7;
        while (!(FLASH->IAPSR & FLASH_IAPSR_EOP)); // wait for write finish

        FLASH->CR2 &= (uint8_t)(~FLASH_CR2_OPT);	// lock back
        FLASH->NCR2 |= FLASH_NCR2_NOPT;
    }*/

    // Setup buzzer
    // For buzzer working you must set option bytes: AFR7 - port D4 alternate function = BEEP and LSI_EN - LSI clock enable as CPU clock source
    // You can do this e.g. from graphic STVP
    BEEP->CSR &= ~BEEP_CSR_BEEPEN; // Disable Buzzer
    BEEP->CSR &= ~BEEP_CSR_BEEPDIV; // Clear DIV register
    BEEP->CSR |= BEEP_CALIBRATION_DEFAULT; // Set register with default calibration
    BEEP->CSR &= ~BEEP_CSR_BEEPSEL; // Clear SEL register
    BEEP->CSR |= BEEP_FREQUENCY_2KHZ; // Set frequency of buzzer to 2kHz
}

void beeper_on()
{
    if (beeper_enabled) {
        BEEP->CSR |= BEEP_CSR_BEEPEN;
    }
}

void beeper_off()
{
    BEEP->CSR &= ~BEEP_CSR_BEEPEN;
}

void beeper_toggle()
{
    if (BEEP->CSR & BEEP_CSR_BEEPEN) {
        beeper_off();
    } else {
        beeper_on();
    }
}
