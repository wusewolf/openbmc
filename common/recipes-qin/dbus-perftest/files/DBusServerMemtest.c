/*
 * Copyright 2014-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <gio/gio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

const gchar introspection_xml[] =
  "<!DOCTYPE node PUBLIC" \
  " \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" " \
  " \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">" \
  "<node>"
  "  <interface name='org.openbmc.TestInterface'>"
  "    <method name='HelloWorld'>"
  "      <arg type='s' name='greeting' direction='in'/>"
  "      <arg type='s' name='response' direction='out'/>"
  "    </method>"
  "  </interface>"
  "</node>";

static GDBusNodeInfo *introspection_data = NULL;
static int objectNum = 1;
static char objectName[80];

static void handle_method_call(GDBusConnection       *connection,
                               const gchar           *sender,
                               const gchar           *object_path,
                               const gchar           *interface_name,
                               const gchar           *method_name,
                               GVariant              *parameters,
                               GDBusMethodInvocation *invocation,
                               gpointer               user_data) {

  if (g_strcmp0(method_name, "HelloWorld") == 0) {
    // sending greetings with other applications
    const gchar *greeting;
    g_variant_get(parameters, "(&s)", &greeting);

    gchar *response;
    response = g_strdup_printf("You greeted me with '%s'. Thanks!", greeting);
    g_dbus_method_invocation_return_value(invocation,
                                          g_variant_new ("(s)", response));
    g_free(response);
  }
}

static const GDBusInterfaceVTable interface_vtable = {handle_method_call, NULL, NULL};

static void on_bus_acquired(GDBusConnection *connection,
                            const gchar     *name,
                            gpointer         user_data) {
  guint registration_id;
  int i;
  for (i = 0; i < objectNum; i++) {
    char buf[20];
    sprintf(buf, "%d", i);
    strcpy(objectName, "/org/openbmc/test");
    strcat(objectName, buf);
    registration_id =
      g_dbus_connection_register_object(connection,
                                        objectName,
                                        introspection_data->interfaces[0],
                                        &interface_vtable,
                                        NULL,  /* user_data */
                                        NULL,  /* user_data_free_func */
                                        NULL); /* GError** */
    g_assert(registration_id > 0);
  }
}

static void on_name_acquired (GDBusConnection *connection,
                              const gchar     *name,
                              gpointer         user_data) {
}

static void on_name_lost (GDBusConnection *connection,
                          const gchar     *name,
                          gpointer         user_data) {
  exit(1);
}

/**
 * The first input is used to specify the number of object to be
 * created.
 */
int main (int argc, char *argv[]) {
  if (argc >= 2) objectNum = (int) strtol(argv[1], NULL, 10);
  if (objectNum < 0 || objectNum > 100000) objectNum = 1;
  guint owner_id;
  GMainLoop *loop;
  GError *error = NULL;

  // build introspection data structures from xml
  introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
  g_assert(introspection_data != NULL);

  // reserve bus name
  owner_id = g_bus_own_name(G_BUS_TYPE_SESSION,
                            "org.openbmc.TestServer",
                            G_BUS_NAME_OWNER_FLAGS_NONE,
                            on_bus_acquired,
                            on_name_acquired,
                            on_name_lost,
                            NULL,
                            NULL);

  loop = g_main_loop_new(NULL, FALSE);
  g_main_loop_run(loop);

  g_bus_unown_name(owner_id);
  g_dbus_node_info_unref(introspection_data);

  return 0;
}
