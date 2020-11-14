/***
  This file is part of PulseAudio.

  Copyright 2020 Soroush Faghihi Kashani <soroush.fk@gmail.com>

  PulseAudio is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  PulseAudio is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with PulseAudio; if not, see <http://www.gnu.org/licenses/>.
***/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulsecore/core.h>
#include <pulsecore/core-util.h>
#include <pulsecore/dbus-util.h>
#include <pulsecore/protocol-dbus.h>
#include <pulsecore/dbus-shared.h>
#include <pulsecore/macro.h>
#include <pulsecore/module.h>
#include <pulsecore/modargs.h>
#include <pulsecore/shared.h>

#include "bluez5-util.h"

PA_MODULE_AUTHOR("Soroush Faghihi Kashani");
PA_MODULE_DESCRIPTION("Act as a bluetooth audio device server, optionally handling bluez5 agents. Can act as A2DP Sink and Source device");
PA_MODULE_VERSION(PACKAGE_VERSION);
PA_MODULE_LOAD_ONCE(true);
PA_MODULE_USAGE(
    "handle_agent=<boolean>"
    "a2dp_sink=<boolean>"
    "a2dp_source=<boolean>"
);

static const char* const valid_modargs[] = {
    "handle_agent",
    "a2dp_sink",
    "a2dp_source",
    NULL
};

struct userdata {
    pa_module *module;
    pa_core *core;
    pa_hashmap *connected_device_paths;
    bool handle_agent;
    pa_bluetooth_discovery *discovery;

    pa_hook_slot *transport_state_changed_slot;
    pa_hook_slot *device_unlinked_slot;
    pa_hook_slot *transport_speaker_gain_changed_slot;
    pa_hook_slot *transport_microphone_gain_changed_slot;
    pa_hook_slot *device_connection_changed_slot;
};

pa_hook_result_t device_connection_changed_cb(pa_bluetooth_discovery *y, const pa_bluetooth_device *d, struct userdata *u);
pa_hook_result_t transport_state_changed_cb(pa_bluetooth_discovery *y, const pa_bluetooth_transport *t, struct userdata *u);
pa_hook_result_t device_unlinked_cb(pa_bluetooth_discovery *y, const pa_bluetooth_device *d, struct userdata *u);
pa_hook_result_t transport_microphone_gain_changed_cb(pa_bluetooth_discovery *y, const pa_bluetooth_transport *t, struct userdata *u);
pa_hook_result_t transport_speaker_gain_changed_cb(pa_bluetooth_discovery *y, const pa_bluetooth_transport *t, struct userdata *u);

pa_hook_result_t device_connection_changed_cb(pa_bluetooth_discovery *y, const pa_bluetooth_device *d, struct userdata *u)
{
    pa_log("YAY in device connection changed!");
    return PA_HOOK_OK;
}

pa_hook_result_t transport_state_changed_cb(pa_bluetooth_discovery *y, const pa_bluetooth_transport *t, struct userdata *u)
{
    pa_log("YAY in transport state changed!");
    return PA_HOOK_OK;
}

pa_hook_result_t device_unlinked_cb(pa_bluetooth_discovery *y, const pa_bluetooth_device *d, struct userdata *u)
{
    pa_log("YAY in device unlinked!");
    return PA_HOOK_OK;
}

pa_hook_result_t transport_microphone_gain_changed_cb(pa_bluetooth_discovery *y, const pa_bluetooth_transport *t, struct userdata *u)
{
    pa_log("YAY in transport mic volume changed!");
    return PA_HOOK_OK;
}

pa_hook_result_t transport_speaker_gain_changed_cb(pa_bluetooth_discovery *y, const pa_bluetooth_transport *t, struct userdata *u)
{
    pa_log("YAY in transport speaker volume changed!");
    return PA_HOOK_OK;
}

int pa__init(pa_module *m) {
    struct userdata *u;
    pa_modargs *ma;
    bool handle_agent;
    bool a2dp_sink;
    bool a2dp_source;

    pa_assert(m);

    if (!(ma = pa_modargs_new(m->argument, valid_modargs))) {
        pa_log("failed to parse module arguments.");
        goto fail;
    }

    handle_agent = false;
    if (pa_modargs_get_value_boolean(ma, "handle_agent", &handle_agent) < 0) {
        pa_log("Invalid boolean value for handle_agent parameter");
        goto fail;
    }

    a2dp_sink = true;
    if (pa_modargs_get_value_boolean(ma, "a2dp_sink", &handle_agent) < 0) {
        pa_log("Invalid boolean value for a2dp_sink parameter");
        goto fail;
    }

    a2dp_source = false;
    if (pa_modargs_get_value_boolean(ma, "a2dp_source", &a2dp_source) < 0) {
        pa_log("Invalid boolean value for a2dp_source parameter");
        goto fail;
    }

    if (!(a2dp_source || a2dp_sink)) {
        pa_log("Both Sink and Source cannot be disabled");
        goto fail;
    }

    m->userdata = u = pa_xnew0(struct userdata, 1);
    u->module = m;
    u->core = m->core;
    u->handle_agent = handle_agent;
    u->connected_device_paths = pa_hashmap_new(pa_idxset_string_hash_func, pa_idxset_string_compare_func);
    u->discovery = pa_bluetooth_discovery_get_server(m->core, HEADSET_BACKEND_DISABLE,
                                                     true, a2dp_sink, a2dp_source);

    if (!(u->discovery)) {
        pa_log("Error setting up Bluez5 Discovery Server");
        goto fail;
    }

    u->device_connection_changed_slot =
        pa_hook_connect(pa_bluetooth_discovery_hook(u->discovery, PA_BLUETOOTH_HOOK_DEVICE_CONNECTION_CHANGED),
                        PA_HOOK_NORMAL, (pa_hook_cb_t) device_connection_changed_cb, u);
    u->transport_microphone_gain_changed_slot =
        pa_hook_connect(pa_bluetooth_discovery_hook(u->discovery, PA_BLUETOOTH_HOOK_TRANSPORT_MICROPHONE_GAIN_CHANGED),
                        PA_HOOK_NORMAL, (pa_hook_cb_t) transport_microphone_gain_changed_cb, u);
    u->transport_speaker_gain_changed_slot =
        pa_hook_connect(pa_bluetooth_discovery_hook(u->discovery, PA_BLUETOOTH_HOOK_TRANSPORT_SPEAKER_GAIN_CHANGED),
                        PA_HOOK_NORMAL, (pa_hook_cb_t) transport_speaker_gain_changed_cb, u);
    u->device_unlinked_slot =
        pa_hook_connect(pa_bluetooth_discovery_hook(u->discovery, PA_BLUETOOTH_HOOK_DEVICE_UNLINK),
                        PA_HOOK_NORMAL, (pa_hook_cb_t) device_unlinked_cb, u);
    u->transport_state_changed_slot =
        pa_hook_connect(pa_bluetooth_discovery_hook(u->discovery, PA_BLUETOOTH_HOOK_TRANSPORT_STATE_CHANGED),
                        PA_HOOK_NORMAL, (pa_hook_cb_t) transport_speaker_gain_changed_cb, u);

    pa_log_notice("Happy as a cammel!");

    pa_modargs_free(ma);
    return 0;

fail:
    if (ma)
        pa_modargs_free(ma);
    pa__done(m);
    return -1;
}

void pa__done(pa_module *m) {
    struct userdata *u;

    pa_assert(m);

    if (!(u = m->userdata))
        return;

    if (u->device_connection_changed_slot)
        pa_hook_slot_free(u->device_connection_changed_slot);

    if (u->transport_state_changed_slot)
        pa_hook_slot_free(u->transport_state_changed_slot);

    if (u->device_unlinked_slot)
        pa_hook_slot_free(u->device_unlinked_slot);

    if (u->transport_microphone_gain_changed_slot)
        pa_hook_slot_free(u->transport_microphone_gain_changed_slot);

    if (u->transport_speaker_gain_changed_slot)
        pa_hook_slot_free(u->transport_speaker_gain_changed_slot);

    if (u->connected_device_paths)
        pa_hashmap_free(u->connected_device_paths);

    pa_xfree(u);
}
