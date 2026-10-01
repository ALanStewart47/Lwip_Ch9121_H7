#ifndef __WIZ_INTERFACE_H__
#define __WIZ_INTERFACE_H__

#include "wizchip_conf.h"




typedef enum
{
  WIZCHIP_OK                = 0,           /* Operation completed successfully.  */
  WIZCHIP_ERROR             = 1,           /* Run-time error without case matched*/
  WIZCHIP_ERRORTIMEOUT      = 2,           /* Operation failed with timeout      */
  WIZCHIP_ERRORRESOURCE     = 3,           /* Resource not available.            */
  WIZCHIP_ERRORPARAMETER    = 4,           /* Parameter error.                   */
  WIZCHIP_ERRORNOMEMORY     = 5,           /* Out of memory.                     */
  WIZCHIP_ERRORISR          = 6,           /* Not allowed in ISR context         */
  WIZCHIP_RESERVED          = 0x7FFFFFFF   /* Reserved                           */
}wizchip_status_t;





/**
 * @brief wizchip init check
 *
 * @return wizchip_status_t 
 */
wizchip_status_t wizchip_init_check(uint8_t s_debug);

/**
 * @brief   wizchip init function
 * @param   none
 * @return  none
 */
void wizchip_initialize(void);

/**
 * @brief   print network information
 * @param   none
 * @return  none
 */
void print_network_information(void);

/**
 * @brief   set network information
 * @param   sn: socketid
 * @param   ethernet_buff:
 * @param   net_info: network information struct
 * @return  none
 */
void network_init(uint8_t *ethernet_buff, wiz_NetInfo *conf_info);

/**
 * @brief Check the WIZCHIP version
 */
void wizchip_version_check(void);

/**
 * @brief Ethernet Link Detection
 */
void wiz_phy_link_check(void);

/**
 * @brief Print PHY information
 */
void wiz_print_phy_info(void);
#endif
