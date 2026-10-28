#pragma once

#include "sdkconfig.h"
#ifndef CONFIG_LWIP_IP_FORWARD
#error "CONFIG_LWIP_IP_FORWARD is not enabled. Add CONFIG_LWIP_IP_FORWARD=y to sdkconfig.defaults (required for NAT/AP routing)"
#endif

#ifndef CONFIG_LWIP_IPV4_NAPT
#error "CONFIG_LWIP_IPV4_NAPT is not enabled. Add CONFIG_LWIP_IPV4_NAPT=y to sdkconfig.defaults (required for SoftAP internet sharing)"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void wifi_init(void);

#ifdef __cplusplus
}
#endif