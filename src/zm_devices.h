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
//  Create a new zm_devices - if file is not NULL, it loads devices from it
ZM_ASSET_PRIVATE zm_devices_t *
    zm_devices_new (const char *file);

//  Destroy the zm_devices
ZM_ASSET_PRIVATE void
    zm_devices_destroy (zm_devices_t **self_p);

//  Return the file for zm devices
ZM_ASSET_PRIVATE const char*
zm_devices_file (zm_devices_t *self);

//  Set the file for zm devices
ZM_ASSET_PRIVATE void
zm_devices_set_file (zm_devices_t *self, const char *file);

//  Store devices
ZM_ASSET_PRIVATE int
zm_devices_store (zm_devices_t *self);

ZM_ASSET_PRIVATE void
zm_devices_insert (zm_devices_t *self, zm_proto_t *msg);

ZM_ASSET_PRIVATE zm_proto_t*
zm_devices_lookup (zm_devices_t *self, const char* name);

ZM_ASSET_PRIVATE void
zm_devices_delete (zm_devices_t *self, const char* name);

//  Self test of this class
ZM_ASSET_PRIVATE void
    zm_devices_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
