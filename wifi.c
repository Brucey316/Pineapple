#include "collection.h"

extern AP** APs;
extern int* num_APs;
extern int num_channels;
extern char* wireless_device;

AP createAP(char* name, struct BSSID bssid){
    AP ap;
    ap.bssid = bssid;
    ap.essid = name;
    ap.clients = NULL;
    ap.num_clients = 0;
    ap.keyCaptured = false;
    ap.attacked = 0;
    return ap;
}

void addClient(AP* ap, struct BSSID bssid){
    ap->num_clients ++;
    ap->clients = realloc(ap->clients, sizeof(BSSID) * ap->num_clients);
    memcpy(&(ap->clients[ap->num_clients-1].bssid), &(bssid), sizeof(BSSID));
    ap->clients[ap->num_clients-1].attack_count = 0;
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

    //check if socket error
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

    //if no interfaces found exit
    if(num_wireless_devices == 0){
        free(interface);
        return(NULL);
    }

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
char** getCompatibleChannels(){
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
    //printf("Number of channels found: %d\n", *num_channels);
    
    //get the channel numbers that are compatible
    char** channels = (char**) malloc(sizeof(char*) * (num_channels));
    for(int i = 0; i < num_channels; i++){
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
            char AP_BSSID [18] = "";
            BSSID_TO_STRING(&APs[c][a].bssid, AP_BSSID);
            printf("%-35s (%s): %d\n", APs[c][a].essid, AP_BSSID, APs[c][a].num_clients);
            for(int client = 0; client < APs[c][a].num_clients; client++){
                char client_BSSID[18] = "";
                BSSID_TO_STRING(&APs[c][a].clients[client].bssid, client_BSSID);
                printf("%10sClient: %s\n", "", client_BSSID);
            }
        }
    }
}