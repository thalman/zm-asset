/*  =========================================================================
    zm_asset - zm asset actor

    Copyright (c) the Contributors as noted in the AUTHORS file.  This file is part
    of zmon.it, the fast and scalable monitoring system.                           
                                                                                   
    This Source Code Form is subject to the terms of the Mozilla Public License, v.
    2.0. If a copy of the MPL was not distributed with this file, You can obtain   
    one at http://mozilla.org/MPL/2.0/.                                            
    =========================================================================
*/

#ifndef ZM_ASSET_H_INCLUDED
#define ZM_ASSET_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


//  @interface
//  Create new zm_asset actor instance.
//  @TODO: Describe the purpose of this actor!
//
//      zactor_t *zm_asset = zactor_new (zm_asset, NULL);
//
//  Destroy zm_asset instance.
//
//      zactor_destroy (&zm_asset);
//
//  Enable verbose logging of commands and activity:
//
//      zstr_send (zm_asset, "VERBOSE");
//
//  Start zm_asset actor.
//
//      zstr_sendx (zm_asset, "START", NULL);
//
//  Stop zm_asset actor.
//
//      zstr_sendx (zm_asset, "STOP", NULL);
//
//  This is the zm_asset constructor as a zactor_fn;
ZM_ASSET_EXPORT void
    zm_asset_actor (zsock_t *pipe, void *args);

//  Self test of this actor
ZM_ASSET_EXPORT void
    zm_asset_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
