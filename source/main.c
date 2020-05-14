/**
 *  @file       main.c
 *
 *  @date       2020.05.14
 *  @author     Stulov Tikhon (kudesnick@inbox.ru)
 *
 *  @brief      Main file
 *  @details    Emulator of ds3231 on nucleo
 *
 *  @mainpage   Emulator of ds3231 on nucleo
 */

#include <stdio.h>

#if !defined(__CC_ARM) && defined(__ARMCC_VERSION) && !defined(__OPTIMIZE__)
    /*
    Without this directive, it does not start if -o0 optimization is used and the "main"
    function without parameters.
    see http://www.keil.com/support/man/docs/armclang_mig/armclang_mig_udb1499267612612.htm
    */
    __asm(".global __ARM_use_no_argv\n\t" "__ARM_use_no_argv:\n\t");
#endif

/**
 * @brief   Main function
 * @details Init peripherial modules and starting superloop
 *
 * @return  Error code
 */
int main(void)
{
    for(;;)
    {
        
    }
}

/***************************************************************************************************
 *                                       END OF FILE
 **************************************************************************************************/
