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
    return g_application_run(G_APPLICATION(sm_app_new()), argc, argv);
}
