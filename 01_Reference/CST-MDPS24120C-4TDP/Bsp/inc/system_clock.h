/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file system_clock.h
 *
 * @par dependencies
 * - 
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of clock handle
 *
 * Processing flow:
 * call directly.
 *
 * @version    V1.0     2025-04-29      ALan 
 * @note    
 *              1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef __SYSTEM_CLOCK_H__
#define __SYSTEM_CLOCK_H__

 //******************************** Includes *********************************//
 #include "main.h"
 #include <stdint.h>              
 #include <stdio.h>
 #include "systick.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
typedef enum
{
    EPRST      = 0 ,        /*!< external PIN reset flags */
    PORRST         ,        /*!< power reset flags */
    SWRST          ,        /*!< software reset flags */
    FWDGTRST       ,        /*!< FWDGT reset flags */
    WWDGTRST       ,        /*!< WWDGT reset flags */
    LPRST                   /*!< low-power reset flags */
}reset_reason;
//******************************** Defines **********************************//

//******************************** Declaring ********************************//
void peripheral_clock_init(void);
uint8_t get_reset_flag(void);
//******************************** Declaring ********************************//

#endif /*__SYSTEM_CLOCK_H__*/

