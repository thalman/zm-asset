/*  =========================================================================
    zm_devices - Devices API

    Copyright (c) the Contributors as noted in the AUTHORS file.  This file is part
    of zmon.it, the fast and scalable monitoring system.                           
                                                                                   
    This Source Code Form is subject to the terms of the Mozilla Public License, v.
    2.0. If a copy of the MPL was not distributed with this file, You can obtain   
    one at http://mozilla.org/MPL/2.0/.                                            
    =========================================================================
*/

/*
@header
    zm_devices - Devices API
@discuss
@end
*/

#include "zm_asset_classes.h"

//  Structure of our class

struct _zm_devices_t {
    zhashx_t *devices;
    char *file;
};


//  --------------------------------------------------------------------------
//  Create a new zm_device
zm_devices_t *
zm_devices_new (const char *file)
{
    zm_devices_t *self = (zm_devices_t *) zmalloc (sizeof (zm_devices_t));
    assert (self);
    //  Initialize class properties here
    self->devices = zhashx_new ();
    assert (self->devices);
    zhashx_set_destructor (self->devices, (void(*)(void**)) zm_proto_destroy);

    if (!file)
        return self;

    self->file = strdup (file);
    zconfig_t *root = zconfig_load (file);
    if (!root) {
        zsys_error ("Fail to load file %s: %s", file, strerror (errno));
        goto fail;
    }

    zconfig_t *item = zconfig_child (root);
    while (item) {
        zm_proto_t *dev = zm_proto_new_zpl (item);
        zhashx_insert (self->devices, zm_proto_device (dev), (void*) dev);
        item = zconfig_next (item);
    }

    zconfig_destroy (&root);
    return self;
fail:
    zconfig_destroy (&root);
    zm_devices_destroy (&self);
    return NULL;
}

//  --------------------------------------------------------------------------
//  Destroy the zm_devices
int
zm_devices_store (zm_devices_t *self);

void
zm_devices_destroy (zm_devices_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zm_devices_t *self = *self_p;
        //  Free class properties here

        zhashx_destroy (&self->devices);
        zstr_free (&self->file);
        zhashx_destroy (&self->devices);
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}

const char*
zm_devices_file (zm_devices_t *self)
{
    assert (self);
    return (const char*) self->file;
}

void
zm_devices_set_file (zm_devices_t *self, const char *file)
{
    assert (self);
    zstr_free (&self->file);
    self->file = strdup (file);
}

int
zm_devices_store (zm_devices_t *self)
{
    assert (self);
    if (self->file) {
        zconfig_t *root = zconfig_new ("root", NULL);
        zm_proto_t *device = (zm_proto_t*) zhashx_first (self->devices);
        while (device) {
            zm_proto_zpl (device, root);
            device = (zm_proto_t*) zhashx_next (self->devices);
        }
        zconfig_save (root, self->file);
        zconfig_destroy (&root);
    }
    return 0;

}

//  --------------------------------------------------------------------------
//  Destroy the zm_devices

void
zm_devices_insert (zm_devices_t *self, zm_proto_t *msg)
{
    assert (self);

    const char *device = zm_proto_device (msg);
    // zm_proto_t will be overwritten on another mlm_client_recv
    // so duplicate it
    zm_proto_t *dev = zm_proto_dup (msg);

    // TODO
    // see: zm-proto issue#1, zhash inside message DOES NOT own memory
    //      we need to find a solution
    //zm_proto_aux_insert (msg, "x-zm-devices-time", "%zu", (uint64_t) zclock_mono ());
    zhashx_update (self->devices, device, (void*)dev);
}

zm_proto_t*
zm_devices_lookup (zm_devices_t *self, const char* name)
{
    assert (self);
    assert (name);

    //TODO:
    //zm_devices_gc (self);
    return (zm_proto_t*) zhashx_lookup (self->devices, name);
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
zm_devices_test (bool verbose)
{
    zsys_init ();
    printf (" * zm_devices: ");

    //  @selftest
    //  Simple create/destroy test
    int r = zsys_dir_create (".test", NULL);
    assert (r == 0);
    zdir_t *dir = zdir_new (".test", NULL);
    assert (dir);

    zm_devices_t *self = zm_devices_new (NULL);
    assert (self);

    assert (!zm_devices_lookup (self, "some"));
    zm_proto_t *dev = zm_proto_new ();
    assert (dev);
    zm_proto_encode_device (
        dev,
        "device1",
        zclock_mono (),
        10000,
        NULL);
    zm_devices_insert (self, dev);
    zm_proto_encode_device (
        dev,
        "device2",
        zclock_mono (),
        10000,
        NULL);
    zm_devices_insert (self, dev);
    zm_proto_encode_device (
        dev,
        "device3",
        zclock_mono (),
        10000,
        NULL);
    zm_devices_insert (self, dev);
    zm_proto_destroy (&dev);

    assert (zm_devices_lookup (self, "device1"));
    assert (zm_devices_lookup (self, "device2"));
    assert (zm_devices_lookup (self, "device3"));

    zm_devices_set_file (self, ".test/devices.zpl");
    r = zm_devices_store (self);
    assert (r == 0);

    zm_devices_t *devices2 = zm_devices_new (".test/devices.zpl");
    assert (devices2);
    assert (streq (zm_devices_file (self), zm_devices_file (devices2)));

    assert (zm_devices_lookup (devices2, "device1"));
    assert (zm_devices_lookup (devices2, "device2"));
    assert (zm_devices_lookup (devices2, "device3"));

    zm_proto_t *device3_old = zm_devices_lookup (self, "device3");
    zm_proto_t *device3_new = zm_devices_lookup (self, "device3");
    assert (streq (zm_proto_device (device3_old), zm_proto_device (device3_new)));

    zm_devices_destroy (&self);
    zm_devices_destroy (&devices2);

    zdir_remove (dir, true);
    zdir_destroy (&dir);

    //  @end
    printf ("OK\n");
}
