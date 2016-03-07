/*
 * scarlettmixer - GTK+ mixer for scarlett audio interfaces.
 * Copyright (c) 2016 Martin Roesch <martin.roesch79@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sm-app.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

int main(int argc, char *argv[]) {
    SmApp *app;
    int status;

#ifndef NDEBUG
    /* Use local GSettings schema directory.
     * configure with --enable-debug switch to enable this section.
     */
    g_setenv("GSETTINGS_SCHEMA_DIR", "./build/src/", FALSE);
#endif
    app = sm_app_new();
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
