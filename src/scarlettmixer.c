/*
 ============================================================================
 Name        : scarlettmixer.c
 Author      : Martin Rösch
 Version     :
 Copyright   : Copyright (C) 2015
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "sm-app.h"

int main(int argc, char *argv[]) {
    SmApp *app;
    int status;

    app = sm_app_new();
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
