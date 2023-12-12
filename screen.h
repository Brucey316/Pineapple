#ifndef SCREEN_H
#define SCREEN_H

#include "common_dependancies.h"
#include "wifi.h"
#include "WPA_attack.h"
#include <curses.h>
#include <signal.h>

#define CP_DEF          1
#define CP_ATTACK       2
#define CP_SUCCESS      3
#define CP_UNSUCCESS    4
#define CP_SCANNING     5

void init_screen(int, int);
void tear_down();
void print_data(int active_channel, BSSID* ap, BSSID* Client);
void print_menu(int active_channel);
void wcprintw(WINDOW* win, attr_t attr, char* str);
void mvwcprintw(WINDOW* win, attr_t attr, int y, int x, char* str);

#endif