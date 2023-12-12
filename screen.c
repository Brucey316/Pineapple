#include "screen.h"

extern AP** APs;
extern int* num_APs;
extern char** channels;
extern int num_channels;
extern WINDOW* menu;
extern WINDOW* data;

int step = STEP_SCANNING;

void handle_size(int signal){
    endwin();
    refresh();
}
void init_screen(int max_height, int max_width){
    signal(SIGWINCH, handle_size);
    initscr();
    if(!has_colors()){
        printf("ERROR: Terminal does not support colors!");
        exit(1);
    }
    else{
        start_color();
        init_pair(CP_DEF, COLOR_WHITE, COLOR_BLACK);
        init_pair(CP_ATTACK, COLOR_RED, COLOR_BLACK);
        init_pair(CP_SUCCESS, COLOR_GREEN, COLOR_BLACK);
        init_pair(CP_UNSUCCESS, COLOR_YELLOW, COLOR_BLACK);
        init_pair(CP_SCANNING, COLOR_CYAN, COLOR_BLACK);
    }
    getmaxyx(stdscr, max_height, max_width);
    if(max_height < 10 || max_width < 10){
        printf("ERRROR: WINDOW SIZE TOO SMALL\n");
        exit(1);
    }
    menu = newwin(max_height, max_width/2, 0, 0);
    data = newwin(max_height, max_width/2, 0, max_width/2);
    refresh();
}
void tear_down(){
    if(menu) delwin(menu);
    if(data) delwin(data);
    endwin();
}
void print_data(int active_channel, BSSID* ap, BSSID* client){
    //clear screen
    wclear(data);
    //move cursor to top of screen
    wmove(data, 0,0);
    //print attacking mode w/ or w/o a client if applicable
    if(ap != NULL){
        //save string format of bssids
        char ap_bssid_str[BSSID_LENGTH] = "";
        BSSID_TO_STRING(ap,ap_bssid_str);
        char attacking_label[55] = "";
        if(client != NULL){
            char client_bssid_str[BSSID_LENGTH] = "";
            BSSID_TO_STRING(client,client_bssid_str);
            sprintf(attacking_label, "Attacking %s using %s\n", ap_bssid_str, client_bssid_str);
        }
        else
            sprintf(attacking_label, "Attacking %s without a client\n", ap_bssid_str);
        wcprintw(data, CP_ATTACK, attacking_label);
    }
    //start printing data
    AP* ap_data = APs[active_channel];
    for(int a = 0; a < num_APs[active_channel]; a++){
        char ap_bssid[BSSID_LENGTH] = "";
        BSSID_TO_STRING(&ap_data[a].bssid, ap_bssid);
        char ap_title[51] = "";
        sprintf(ap_title, "%-35s(%s)\n", ap_data[a].essid, ap_bssid);
        //if AP is being attacked, P in CP_ATTACK
        if(ap != NULL && memcmp(&ap_data[a].bssid, ap, sizeof(BSSID)) == 0)
            wcprintw(data, COLOR_PAIR(CP_ATTACK), ap_title);
        //if key already captured
        else if(ap_data[a].keyCaptured)
            wcprintw(data, COLOR_PAIR(CP_SUCCESS), ap_title);
        //if already attacked but no key yet
        else if(ap_data[a].attacked)
            wcprintw(data, COLOR_PAIR(CP_UNSUCCESS), ap_title);
        //print ap normally if not being attacked
        else wprintw(data, "%-35s(%s)\n", ap_data[a].essid, ap_bssid);
        //save space and dont print clients if key captured
        if(ap_data[a].keyCaptured) continue;
        //if key not captured, print all the clients
        for(int c = 0; c < ap_data[a].num_clients; c++){
            char client_bssid[BSSID_LENGTH] = "";
            BSSID_TO_STRING(&ap_data[a].clients[c].bssid, client_bssid);
            char client_title[25] = "";
            sprintf(client_title, "\t%s\n", client_bssid);
            //add color to client if being attacked
            if(client != NULL && memcmp(&ap_data[a].clients[c].bssid, client, sizeof(BSSID)) == 0){
                wcprintw(data, COLOR_PAIR(CP_ATTACK), client_title);
            }
            else if(ap_data[a].clients[c].attacked){
                wcprintw(data, COLOR_PAIR(CP_UNSUCCESS), client_title);
            }
            //print client normally if not being attacked
            else wprintw(data, "\t%s\n", client_bssid);
        }
    }
    if(step == STEP_HANDSHAKE_CHECKING){
        wprintw(data, "CHECKING FOR HANDSHAKE\n");
    }
    if(step == STEP_SAVING_PCAP){
        wprintw(data, "CHECKING FOR HANDSHAKE\n");
        wprintw(data, "HANDSHAKE FOUND... SAVING PCAP\n");
    }
    wrefresh(data);
}
void print_menu(int active_channel){
    wclear(menu);
    char title[30] = "";
    sprintf(title, "Scanning on channel: %s", channels[active_channel]);
    mvwcprintw(menu, COLOR_PAIR(CP_SCANNING), 0, 0, title);
    for(int c = 0; c < num_channels; c++){
        if(c == active_channel && has_colors()){
            char channel[30] = "";
            sprintf(channel, "Channel: %s", channels[c]);
            mvwcprintw(menu, COLOR_PAIR(CP_SCANNING) ,c+1, 0, channel);
            continue;
        }
        mvwprintw(menu, c+1, 0, "Channel: %s", channels[c]);
    }
    wrefresh(menu);
}
//custom print functions to handle attribute changes
void wcprintw(WINDOW* win, attr_t attr, char* str){
    wattroff(win, COLOR_PAIR(CP_DEF));
    wattron(win, attr);
    wprintw(win, "%s", str);
    wattroff(win, attr);
    wattron(win, COLOR_PAIR(CP_DEF));
}
//custom print functions to handle attribute changes
void mvwcprintw(WINDOW* win, attr_t attr, int y, int x, char* str){
    wattroff(win, COLOR_PAIR(CP_DEF));
    wattron(win, attr);
    mvwprintw(win, y, x, "%s", str);
    wattroff(win, attr);
    wattron(win, COLOR_PAIR(CP_DEF));
}