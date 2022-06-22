#include "stm8s.h"
#include "stm8s_4segment.h"

#define PORT_BUTTONS (GPIOC) // Port tlačítek
#define BUTTON1 (PIN_7)      // SET
#define BUTTON2 (PIN_6)      // SAVE TO MEMORY
#define BUTTON3 (PIN_5)      // RESET
#define BUTTON4 (PIN_4)      // BROWSE MEMORY

GPIO_Pin_TypeDef segButtons[4] = {BUTTON1, BUTTON2, BUTTON3, BUTTON4}; // Tlačítka pro nastavování času

// proměnné pro čas
uint16_t sec = 0;
uint16_t min = 0;
uint32_t timeUnit = 0;
bool state = FALSE; // Stav časovače (FALSE = neaktivní, TRUE = odpočítává)

// Změna stavu odpočtu - je možné odpočet spustit nebo pozastavit
void changeState()
{
    if (state == FALSE)
    {
        state = TRUE;
    }
    else
    {
        state = FALSE;
    }
}

// Přerušení
INTERRUPT_HANDLER(EXTI_PORTC_IRQHandler, 5)
{
    if (state == 0)
    {
        adjustTime();
    } // Pokud je stav odpočtu neaktivní, je možné nastavit čas na odpočet

    if (buttonPressed(BUTTON5))
    {
        changeState();
    } // Změna stavu odpočtu při zmáčknutí tlačítka 5

    if (buttonPressed(BUTTON6))
    {
        reset();
    } // Resetování odpočtu při zmáčknutí tlačítka 6
}

int main(void)
{
    GPIO_DeInit;                                   // Deinicializace portů
    TIM4_DeInit;                                   // Deinicializace časovače
    CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1); // taktovat MCU na 16MHz
    Serial.begin(9600);                            // Inicializace Serial monitoru
    display.init();                                // Inicializace displeje
    display.write(0000);                           // Zápis 0000 na začátek
    for (uint8_t i = 0; i < 4; i++)                // Inicializace tlačítek
    {
        GPIO_Init(PORT_BUTTONS, segButtons[i], GPIO_MODE_IN_PU_NO_IT);
    }

    // Nastavení přerušení
    EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOC, EXTI_SENSITIVITY_FALL_ONLY);
    ITC_SetSoftwarePriority(ITC_IRQ_PORTC, ITC_PRIORITYLEVEL_0);
    enableInterrupts();

    /* GPIO_Init(PORT_BUTTONS, , GPIO_MODE_IN_PU_NO_IT); // Tlačítko (Save to memory)
    GPIO_Init(PORT_BUTTONS, pin_tlac3, GPIO_MODE_IN_PU_NO_IT); // Tlačítko (RESET)
    GPIO_Init(PORT_BUTTONS, pin_tlac4, GPIO_MODE_IN_PU_NO_IT); // Tlačítko (Browse memory) */

    // časovač
    // prescale for 1 ms - P = (16 * 10 '6) / (500 * 2 '8) = 128
    // top for 1ms - C = (16 * 10 '6) / (1000 * 62,5) = 250
    TIM4_TimeBaseInit(TIM4_PRESCALER_128, 250); // Inicializace časovače
    TIM4_Cmd(ENABLE);                           // Start časovače
    printf("Proběhla incializace\n");

    while (1)
    {
        if (GPIO_ReadInputPin(port_tlac1, pin_tlac1) == SET) // tlačítko č. 1 (SET)
        {
            TIM4_Cmd(ENABLE);
            printf("tlač. SET\n");
        }
        else if (GPIO_ReadInputPin(port_tlac2, pin_tlac2) == RESET) // tlačítko č. 2 (SAVE TIME TO MEMORY)
        {
            char mezicas[] = {{min, sec}};
            printf("tlač. Mezičas\n");
        }
        else if (GPIO_ReadInputPin(port_tlac3, pin_tlac3) == SET) // tlačítko č. 3 (RESET)
        {
            TIM4_Cmd(DISABLE);
            TIM4_ClearFlag(TIM4_FLAG_UPDATE);
            printf("tlač. RESET\n");
            nop();
        }

        else if (GPIO_ReadInputPin(port_tlac4, pin_tlac4) == RESET) // tlačítko č. 4 (BROWSE TIME HISTORY IN MEMORY)
        {
            printf("tlač. Prohlížení časů\n");
            nop();
        }

        if (TIM4_GetFlagStatus(TIM4_FLAG_UPDATE) == SET)
        {
            TIM4_ClearFlag(TIM4_FLAG_UPDATE);
            timeUnit++;
            status = TRUE;
        }

        if (timeUnit == 500)
        {
            timeUnit = 0;
            sec += 1;
        }

        if (sec > 59)
        {
            sec = 0;
            if (min != 99)
            {
                min++;
            }
        }

        display.clock(min, sec);     // Tisk na displej
        printf("%d:%d\n", min, sec); // Tisk do Serial monitoru

        if (min > 99)
        {
            printf("autoreset\n");
            status = FALSE;
            TIM4_ClearFlag(TIM4_FLAG_UPDATE);
        }
    }
}