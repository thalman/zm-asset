/*  =========================================================================
    zm_devices - Devices API

    Copyright (c) the Contributors as noted in the AUTHORS file.  This file is part
    of zmon.it, the fast and scalable monitoring system.                           
                                                                                   
    This Source Code Form is subject to the terms of the Mozilla Public License, v.
    2.0. If a copy of the MPL was not distributed with this file, You can obtain   
    one at http://mozilla.org/MPL/2.0/.                                            
    =========================================================================
*/

#ifndef ZM_DEVICES_H_INCLUDED
#define ZM_DEVICES_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  Create a new zm_devices
ZM_ASSET_PRIVATE zm_devices_t *
    zm_devices_new (const char *file);

//  Destroy the zm_devices
ZM_ASSET_PRIVATE void
    zm_devices_destroy (zm_devices_t **self_p);

//  Self test of this class
ZM_ASSET_PRIVATE void
    zm_devices_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
