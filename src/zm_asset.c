/*  =========================================================================
    zm_asset - zm asset actor

    Copyright (c) the Contributors as noted in the AUTHORS file.  This file is part
    of zmon.it, the fast and scalable monitoring system.                           
                                                                                   
    This Source Code Form is subject to the terms of the Mozilla Public License, v.
    2.0. If a copy of the MPL was not distributed with this file, You can obtain   
    one at http://mozilla.org/MPL/2.0/.                                            
    =========================================================================
*/

/*
@header
    zm_asset - zm asset actor
@discuss
@end
*/

#include "zm_asset_classes.h"

//  Structure of our actor

struct _zm_asset_t {
    zsock_t *pipe;              //  Actor command pipe
    zpoller_t *poller;          //  Socket poller
    bool terminated;            //  Did caller ask us to quit?
    bool verbose;               //  Verbose logging enabled?
    //  TODO: Declare properties
    zconfig_t *config;          //  Server configuration
    mlm_client_t *client;       //  Malamute client
    zhash_t *consumers;         //  List of streams to subscribe
    zm_proto_t *msg;            //  Last received message
    zm_devices_t *devices;      //  List of devices to maintain
};


//  --------------------------------------------------------------------------
//  Create a new zm_asset instance

static zm_asset_t *
zm_asset_new (zsock_t *pipe, void *args)
{
    zm_asset_t *self = (zm_asset_t *) zmalloc (sizeof (zm_asset_t));
    assert (self);

    self->pipe = pipe;
    self->terminated = false;
    self->poller = zpoller_new (self->pipe, NULL);
    self->devices = zm_devices_new (NULL);

    //  TODO: Initialize properties
    self->config = NULL;
    self->consumers = NULL;
    self->msg = NULL;
    self->client = mlm_client_new ();
    assert (self->client);
    zpoller_add (self->poller, mlm_client_msgpipe (self->client));

    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the zm_asset instance

static void
zm_asset_destroy (zm_asset_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        zm_asset_t *self = *self_p;

        //  TODO: Free actor properties
        zconfig_destroy (&self->config);
        zhash_destroy (&self->consumers);
        zm_proto_destroy (&self->msg);
        mlm_client_destroy (&self->client);
        zpoller_destroy (&self->poller);

        zm_devices_store (self->devices);
        zm_devices_destroy (&self->devices);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}

static const char*
zm_asset_cfg_endpoint (zm_asset_t *self)
{
    assert (self);
    if (self->config) {
        return zconfig_resolve (self->config, "malamute/endpoint", NULL);
    }
    return NULL;
}

static const char*
zm_asset_cfg_name (zm_asset_t *self)
{
    assert (self);
    if (self->config) {
        return zconfig_resolve (self->config, "server/name", NULL);
    }
    return NULL;
}

static const char *
zm_asset_cfg_producer (zm_asset_t *self) {
    assert (self);
    if (self->config) {
        return zconfig_resolve (self->config, "malamute/producer", NULL);
    }
    return NULL;
}

static const char *
zm_asset_cfg_file (zm_asset_t *self) {
    assert (self);
    if (self->config) {
        return zconfig_resolve (self->config, "server/file", NULL);
    }
    return NULL;
}

static const char*
zm_asset_cfg_consumer_first (zm_asset_t *self) {
    assert (self);
    zhash_destroy (&self->consumers);
    self->consumers = zhash_new ();
    
    zconfig_t *cfg = zconfig_locate (self->config, "malamute/consumer");
    if (cfg) {
        zconfig_t *child = zconfig_child (cfg);
        while (child) {
            zhash_insert (self->consumers, zconfig_name (child), zconfig_value (child));
            child = zconfig_next (child);
        }
    }

    return (const char*) zhash_first (self->consumers);
}

static const char*
zm_asset_cfg_consumer_next (zm_asset_t *self) {
    assert (self);
    assert (self->consumers);
     return (const char*) zhash_next (self->consumers);
}

static const char*
zm_asset_cfg_consumer_stream (zm_asset_t *self) {
    assert (self);
    assert (self->consumers);
     return zhash_cursor (self->consumers);
}

static int
zm_asset_connect_to_malamute (zm_asset_t *self)
{
    if (!self->config) {
        zsys_warning ("zm-asset: no CONFIGuration provided, there is nothing to do");
        return -1;
    }

    const char *endpoint = zm_asset_cfg_endpoint (self);
    const char *name = zm_asset_cfg_name (self);

    if (!self->client) {
        self->client = mlm_client_new ();
        zpoller_add (self->poller, mlm_client_msgpipe (self->client));
    }

    int r = mlm_client_connect (self->client, endpoint, 5000, name);
    if (r == -1) {
        zsys_warning ("Can't connect to malamute endpoint %", endpoint);
        return -1;
    }

    if (zm_asset_cfg_producer (self)) {
        r = mlm_client_set_producer (self->client, zm_asset_cfg_producer (self));
        if (r == -1) {
            zsys_warning ("Can't setup publisher on stream %", zm_asset_cfg_producer (self));
            return -1;
        }
    }

    const char *pattern = zm_asset_cfg_consumer_first (self);
    while (pattern) {
        const char *stream = zm_asset_cfg_consumer_stream (self);
        r = mlm_client_set_consumer (self->client, stream, pattern);
        if (r == -1) {
            zsys_warning ("Can't setup consumer %s/%s", stream, pattern);
            return -1;
        }
        pattern = zm_asset_cfg_consumer_next (self);
    }
    return 0;
}

//  Start this actor. Return a value greater or equal to zero if initialization
//  was successful. Otherwise -1.

static int
zm_asset_start (zm_asset_t *self)
{
    assert (self);

    int r = zm_asset_connect_to_malamute (self);
    if (r == -1)
        return r;

    return 0;
}


//  Stop this actor. Return a value greater or equal to zero if stopping 
//  was successful. Otherwise -1.

static int
zm_asset_stop (zm_asset_t *self)
{
    assert (self);

    //  TODO: Add shutdown actions
    zpoller_remove (self->poller, self->client);
    mlm_client_destroy (&self->client);
    zm_devices_store (self->devices);

    return 0;
}

//  Config message, second argument is string representation of config file
static int
zm_asset_config (zm_asset_t *self, zmsg_t *request)
{
    assert (self);
    assert (request);

    char *str_config = zmsg_popstr (request);
    if (str_config) {
        zconfig_t *foo = zconfig_str_load (str_config);
        if (foo) {
            zconfig_destroy (&self->config);
            self->config = foo;
            if (zm_asset_cfg_file (self)) {
                if (!zm_devices_file (self->devices))
                    zm_devices_set_file (self->devices, zm_asset_cfg_file (self));
                zm_devices_store (self->devices);
                zm_devices_destroy (&self->devices);
                self->devices = zm_devices_new (zm_asset_cfg_file (self));
            }
        }
        else {
            zsys_warning ("zm_asset: can't load config file from string");
            return -1;
        }
    }
    else
        return -1;
    return 0;
}


//  Here we handle incoming message from the node

static void
zm_asset_recv_api (zm_asset_t *self)
{
    //  Get the whole message of the pipe in one go
    zmsg_t *request = zmsg_recv (self->pipe);
    if (!request)
       return;        //  Interrupted

    char *command = zmsg_popstr (request);
    if (streq (command, "START"))
        zm_asset_start (self);
    else
    if (streq (command, "STOP"))
        zm_asset_stop (self);
    else
    if (streq (command, "VERBOSE"))
        self->verbose = true;
    else
    if (streq (command, "$TERM"))
        //  The $TERM command is send by zactor_destroy() method
        self->terminated = true;
    else
    if (streq (command, "CONFIG"))
        zm_asset_config (self, request);
    else {
        zsys_error ("invalid command '%s'", command);
        assert (false);
    }
    zstr_free (&command);
    zmsg_destroy (&request);
}

static void
zm_asset_recv_mlm_mailbox (zm_asset_t *self)
{
    assert (self);
    zm_proto_print (self->msg);

    const char *subject = mlm_client_subject (self->client);
    if (streq (subject, "INSERT")) {
        zm_devices_insert (self->devices, self->msg);
        // TODO: there should be a reply, extend zm_proto with OK/ERROR messages?
    }
    else
    if (streq (subject, "DELETE")) {
        const char *device = zm_proto_device (self->msg);
        zm_devices_delete (self->devices, device);
        // TODO: there should be a reply, extend zm_proto with OK/ERROR messages?
        // TODO: it should be announced on ASSET stream
    }
    else
    if (streq (subject, "LOOKUP")) {
        const char *device = zm_proto_device (self->msg);
        zm_proto_t *reply = zm_devices_lookup (self->devices, device);
        if (reply) {
            zmsg_t *zreply = zmsg_new ();
            zm_proto_send (reply, zreply);
            mlm_client_sendto (
                self->client,
                mlm_client_sender (self->client),
                "LOOKUP",
                NULL,
                5000,
                &zreply);
        }
        else {
            zsys_warning ("Can't send error reply now");
        }
    }

}

static void
zm_asset_recv_mlm_stream (zm_asset_t *self)
{
    assert (self);
    zm_proto_print (self->msg);

    if (zm_proto_id (self->msg) != ZM_PROTO_DEVICE) {
        if (self->verbose)
            zsys_warning ("message from sender=%s, with subject=%s os not DEVICE",
            mlm_client_sender (self->client),
            mlm_client_subject (self->client));
        return;
    }
}

static void
zm_asset_recv_mlm (zm_asset_t *self)
{
    assert (self);
    zmsg_t *request = mlm_client_recv (self->client);
    int r = zm_proto_recv (self->msg, request);
    zmsg_destroy (&request);
    if (r != 0) {
        if (self->verbose)
            zsys_warning ("can't read message from sender=%s, with subject=%s",
            mlm_client_sender (self->client),
            mlm_client_subject (self->client));
        return;
    }

    if (streq (mlm_client_command (self->client), "MAILBOX DELIVER"))
        zm_asset_recv_mlm_mailbox (self);
    else
    if (streq (mlm_client_command (self->client), "STREAM DELIVER"))
        zm_asset_recv_mlm_stream (self);
}

//  --------------------------------------------------------------------------
//  This is the actor which runs in its own thread.

void
zm_asset_actor (zsock_t *pipe, void *args)
{
    zm_asset_t * self = zm_asset_new (pipe, args);
    if (!self)
        return;          //  Interrupted

    //  Signal actor successfully initiated
    zsock_signal (self->pipe, 0);

    while (!self->terminated) {
        zsock_t *which = (zsock_t *) zpoller_wait (self->poller, 0);

        if (which == self->pipe)
            zm_asset_recv_api (self);
        else
        if (which == mlm_client_msgpipe (self->client))
            zm_asset_recv_mlm (self);
       //  Add other sockets when you need them.
    }
    zm_asset_destroy (&self);
}

//  --------------------------------------------------------------------------
//  Self test of this actor.

void
zm_asset_test (bool verbose)
{
    printf (" * zm_asset: ");
    //  @selftest
    //  Simple create/destroy test
    // actor test
    zactor_t *zm_asset = zactor_new (zm_asset_actor, NULL);

    zactor_destroy (&zm_asset);
    //  @end

    printf ("OK\n");
}
