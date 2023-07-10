#include "collection.h"

AP** APs;
int* num_APs;
char** channels;
int num_channels;
char* wireless_device;
int scanning_state;

void sig_handler(int signal){
    if(signal == SIGALRM){
        printf("Scanning stopped\n");
        scanning_state = 0;
    }
    else if(signal == SIGUSR1){
        printf("ERROR RUNNING AIRODUMP-NG\n");
        exit(1);
    }
}

int main(int argc, char* argv[]){
    num_channels = 0;
    wireless_device = getWirelessDevice();
    
    //initialize variables and allocate memory
    initVals();
    
    //DEBUG: print available channels
    printf("Channels: %d\n\n", num_channels);
    for(int i = 0; i < num_channels; i++){
        printf("Channel: %s\n", channels[i]);
    }

    //start scanning through channels
    for(int iteration = 1; iteration <= 4; iteration++){
        for(int c = 0; c < num_channels; c++){
            char file_name [50];
            char prefix [50];
            sprintf(prefix, "scanning-%s", channels[c]);
            sprintf(file_name, "scanning-%s-%d", channels[c], iteration);
            
            int airodump_pid = startAirodumpScan(prefix, channels[c]);
            scanning_state=1;

            //setup timeout for the scanning and attacking
            signal(SIGALRM, sig_handler);
            alarm(5);
            
            while(scanning_state){
                //read in the information
                readCSV("channel1-01.csv");
                //attack the APs
                attackAPs(c);
            }
            
            stopAirodumpScan(airodump_pid);
            //read in AP/station data from a CSV
            
        }
    }
    //printReport();
    cleanup();
    return 0;
}
void initVals(){    
    //get available channels
    channels = getCompatibleChannels(&num_channels);
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
    char command[50];
    sprintf(command, "airodump-ng -c %s -w %s -o csv,cap %s", channel, prefix, wireless_device);
    //printf("Command: %s\n", command);
    pid_t pid = fork();
    switch(pid){
        case -1: //error
            printf("Error running command\n");
            exit(1);
            break;
        case 0: //child
            //hide output of scanning
            int dev_null = open("/dev/null", O_WRONLY);
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
    kill(pid, SIGTERM);
    sleep(1);
    waitpid(pid, &status, WNOHANG);
    kill(pid, SIGKILL);
}
int attackAPs(int channel){
    AP* victims = APs[channel];
    for(int a = 0; a < num_APs[channel]; a++){
        if(victims[a].keyCaptured) continue;
        if(victims->attacked < 5 && victims->num_clients > 0){
            printf("Victim found: ");
            PRINT_BSSID(&victims->bssid);
            printf("\n");
            printf("\tClients:\n");
            for(int c = 0; c < victims->num_clients; c++){
                printf("\t");
                PRINT_BSSID(&victims->clients[c]);
                printf("\n");
            }
            victims->attacked++;
        }
    }
    return 0; //if successful attack
}
void readCSV(char* fileName){
    //open up csv
    FILE* csv = fopen(fileName, "r");
    //var holds length of data on a line
    int line_length;
    //initialize buffer
    size_t buffer_sz = 255*sizeof(char);
    char* buffer = malloc(buffer_sz);

    //boolean to distinguish if we are on AP or station data
    bool reading_stations = false;

    //start reading csv line by line
    while( (line_length = getline(&buffer, &buffer_sz, csv)) != -1){
        //skip over blank lines
        if(line_length == 2) continue;

        //printf("\n(%d,%d) %s", line_length, reading_stations, line);

        //reading in the access points
        if(!reading_stations){
            //allocate space for a duplicate of the buffer
            char* line = strdup(buffer);
            //tokenize the line and delimit on ',' since file is CSV
            char* token = strtok(line,",");

            //skip over headers
            //Station MAC header means transition to station data
            if(strcmp(token,"Station MAC") == 0) reading_stations = true;
            else if(strcmp(token, "BSSID") == 0) ;
            //read in AP data
            else read_AP_Data(buffer);

            free(line);
        }  
        //reading in clients
        if(reading_stations){
            read_Station_Data(buffer);
        }
    };

    free(buffer);
    fclose(csv);
}
void read_Station_Data(char* line){
    //var holds the column number
    int tokens = 0;
    //bssid of AP
    struct BSSID bssid;
    memset(&bssid, 0, sizeof(BSSID));
    //bssid of station
    struct BSSID sbssid;
    memset(&sbssid, 0, sizeof(BSSID));
    //tokenized line for CSV
    char* token = strtok(line,",");
    do{
        //read in station BSSID
        if(tokens == 0)
            PACK_BSSID(token, &sbssid);
        //read in station's association
        else if(tokens == 5){
            //if station is not associated to an AP
            if(strcmp(token," (not associated) ") == 0){
                //pack empty BSSID if not associated
                char* temp = "00:00:00:00:00:00";
                PACK_BSSID(temp, &bssid);
            }
            else{
                //load BSSID from CSV into struct
                PACK_BSSID(token+1, &bssid);
            }
        }
        tokens++;
    } while( (token = strtok(NULL,",")) != NULL);

    //find associated AP
    for(int c = 0; c < num_channels; c++){
        for(int a = 0; a < num_APs[c]; a++){
            if(memcmp(&(APs[c][a].bssid), &bssid, sizeof(BSSID)) == 0){
                //Once AP is found, see if it already exists in list
                for(int client = 0; client < APs[c][a].num_clients; client++){
                    if(memcmp( &APs[c][a].clients[client], &sbssid, sizeof(BSSID)) == 0){
                        return;
                    }
                }
                //if not found
                addClient(&APs[c][a],sbssid);
                return;
                //printf("Num clients: %d\n", APs[c][a].num_clients);
            }
        }
    }
    //if AP not in list do nothing
}
void read_AP_Data(char* line){
    //BSSID of AP
    struct BSSID bssid;
    //ESSID of AP
    char* essid = NULL;
    //AP channel
    int channel = 0;
    //column number
    int tokens = 0;
    //tonized line
    char* token = strtok(line,",");

    do{
        //column 1 (bssid)
        if(tokens == 0)
            PACK_BSSID(token, &bssid);
        //column 4 (channel)
        else if(tokens == 3){
            sscanf(token, " %d", &channel);
        }
        //column 14 (essid)
        else if(tokens == 13){
            essid = strdup(token+1);
        }
        tokens++;
    } while( (token = strtok(NULL,",")) != NULL);

    //find out which channel index this AP belongs
    int i;
    for(i = 0; i < num_channels; i++){
        if( atoi(channels[i]) == channel ) break;
    }
    //channel not supported
    if(i == num_channels){
        free(essid);
        return;
    }

    //check if AP already exists in list
    int a = 0;
    for(a = 0; a < num_APs[i]; a++){
        if(memcmp(&APs[i][a],&bssid, sizeof(struct BSSID)) == 0){
            free(essid);
            return;
        }
    }
    //if entry already exists... check to see if essid needs an update
    if(a != num_APs[i]){
        //if essids match prev record, erase new record
        if(strcmp(APs[i][a].essid,essid) == 0)
            free(essid);
        //if new essid is not blank, overwrite old essid with new essid
        else{
            if(strlen(essid) != 0){
                free(APs[i][a].essid);
                APs[i][a].essid = essid;
            }
        }
        return;
    }

    //create new AP entry since it does not exist
    AP ap = createAP(essid, bssid);

    //add to list of APs in that channel
    num_APs[i] ++;
    AP* temp = realloc(APs[i], sizeof(AP) * num_APs[i]);
    if(temp == NULL){
        printf("ERROR: EXPANDING APs FAILED\n");
        exit(1);
    }
    APs[i] = temp;
    APs[i][num_APs[i]-1] = ap;
}


AP createAP(char* name, struct BSSID bssid){
    AP ap;
    ap.essid = name;
    ap.bssid = bssid;
    ap.clients = NULL;
    ap.num_clients = 0;
    ap.keyCaptured = false;
    ap.attacked = 0;
    return ap;
}

void addClient(AP* ap, struct BSSID bssid){
    ap->num_clients ++;
    ap->clients = realloc(ap->clients, sizeof(BSSID) * ap->num_clients);
    memcpy(&(ap->clients[ap->num_clients-1]), &(bssid), sizeof(BSSID));
}

void destroyAP(AP ap){
    free(ap.clients);
    free(ap.essid);
}

char* getWirelessDevice(){
    //init variables
    FILE* commands = NULL;
    size_t buffer_sz = 512*sizeof(char);
    char* buffer = malloc(buffer_sz);
    memset(buffer, 0, buffer_sz);

    //run command through socket
    commands = popen("iw dev", "r");

    //check if command had error
    if(commands == NULL){
        printf("ERROR: 'iw dev' failed\n");
        free(buffer);
        return NULL;
    }

    //grab list of potential wireless devices
    int chars_read = 0;
    int num_wireless_devices = 1;
    char** interface = malloc(sizeof(char*) * num_wireless_devices);
    interface[num_wireless_devices-1] = malloc( sizeof(char) * 50 );
    while( (chars_read = getline(&buffer, &buffer_sz, commands)) != -1 ){
        if(sscanf(buffer, "        Interface %s", interface[num_wireless_devices-1]) != 0){
            num_wireless_devices++;
            interface = realloc(interface, sizeof(char*) * num_wireless_devices);
            interface[num_wireless_devices-1] = malloc( sizeof(char) * 50 );
        } 
    }
    
    //close up socket and buffer
    pclose(commands);
    free(buffer);

    //resize the interfaces list
    num_wireless_devices--;
    free(interface[num_wireless_devices]);
    interface = realloc(interface, sizeof(char*) * num_wireless_devices);
    
    //print wireless device options
    for(int i = 0; i < num_wireless_devices; i++){
        printf("Interface %d: %s\n", i+1, interface[i]);
    }
    printf("\n\n");

    //let users choose which interface they use
    int selection = 0;
    do{
        printf("Choose which interface you want: ");
        char small_buffer = getchar();
        if(small_buffer > 48 && small_buffer <= 48+num_wireless_devices){
            selection = (int) (small_buffer - 48);
        }
        printf("\n");
        fflush(stdin);
    }while(selection == 0);

    //allocate interface name
    char* answer = strdup(interface[selection-1]);
    
    //free up interface list
    printf("Using: %s\n", answer);
    for(int i = 0; i < num_wireless_devices; i++){
        free(interface[i]);
    }
    free(interface);

    //return allocated interface name
    return answer;
}
char** getCompatibleChannels(int* num_channels){
    //create an entry for every channel available
    FILE* commands = NULL;
    size_t buffer_sz = 255*sizeof(char);
    char* buffer = malloc(buffer_sz);
    memset(buffer, 0, buffer_sz);

    //create and execute commannd: iwlist {WIRELESS_DEVICE} channel
    char command[strlen(wireless_device) + strlen("iwlist  channel 2>&1") + 1];
    sprintf(command, "iwlist %s channel 2>&1", wireless_device);
    commands = popen(command, "r");

    //get output from command
    getline(&buffer, &buffer_sz, commands);
    
    //check if buffer is empty
    char error_message[26] = "no frequency information";
    if( strstr(buffer, error_message) != NULL){
        pclose(commands);
        free(buffer);
        return NULL;
    }
    
    //scan for the number of channels compatible with our wireless device
    char scan[strlen(wireless_device) + strlen("   %d channels") + 1];
    sprintf(scan, "%s %%d channels", wireless_device);
    sscanf(buffer, scan, num_channels);
    printf("Scan %s\n", scan);
    printf("Number of channels found %d\n", *num_channels);
    
    //get the channel numbers that are compatible
    char** channels = (char**) malloc(sizeof(char*) * (*num_channels));
    for(int i = 0; i < *num_channels; i++){
        getline(&buffer, &buffer_sz, commands);
        char* channel = malloc(sizeof(char)*4);
        sscanf(buffer,"\tChannel %s", channel);
        channels[i] = channel;
    }

    free(buffer);
    pclose(commands);
    return channels;
}
void printReport(){
    //iterate through each channel
    for(int c = 0; c < num_channels; c++){
        //iterate through each AP on each channel
        for(int a = 0; a < num_APs[c]; a++){
            //print data regarding AP
            printf("%-35s (", APs[c][a].essid);
            PRINT_BSSID(&(APs[c][a].bssid));
            printf("): %d\n", APs[c][a].num_clients);
            for(int client = 0; client < APs[c][a].num_clients; client++){
                printf("%10s","Client:");
                PRINT_BSSID(&(APs[c][a].clients[client]));
                printf("\n");
            }
        }
    }
}