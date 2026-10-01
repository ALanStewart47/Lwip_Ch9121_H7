#ifndef __APP_SLAVE_PROCESSOR_H_
#define __APP_SLAVE_PROCESSOR_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化从机处理器
 * @note  该函数会初始化SPI从机，并启动持续接收
 */
void slave_processor_init(void);


#ifdef __cplusplus
}
#endif

#endif // __APP_SLAVE_PROCESSOR_H_
