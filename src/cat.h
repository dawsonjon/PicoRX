#ifndef __cat__
#define __cat__

#include "settings.h"
#include "xcvr.h"

void process_cat_control(xcvr_settings& settings_to_apply, xcvr_status& status, xcvr& transceiver,
                         s_settings& settings);

#endif
