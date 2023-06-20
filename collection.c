#include "collection.h"

#define WIRELESS_DEVICE "wlan0"
AP** APs;
int* num_APs;
char** channels;
int num_channels;

int main(int argc, char* argv[]){
    num_channels = 0;

    //initialize variables and allocate memory
    initVals();
    
    //DEBUG: print available channels
    printf("Channels: %d\n\n", num_channels);
    for(int i = 0; i < num_channels; i++){
        printf("Channels: %s\n", channels[i]);
    }

    //read in AP/station data from a CSV
    readCSV("channel1-01.csv");
    printReport();

    //Once done, start freeing memory
    for(int c = 0; c < num_channels; c++){
        for(int a = 0; a < num_APs[c]; a++)
            destroyAP(APs[c][a]);
        free(APs[c]);
    }
    free(APs);
    free(num_APs);

    for(int c = 0; c < num_channels; c++)
        free(channels[c]);
    free(channels);
    
    return 0;
}
void initVals(){    
    //get available channels
    channels = getCompatibleChannels(&num_channels);

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
            PACK_BSSID(token, &bssid);
        //read in station's association
        else if(tokens == 5){
            //if station is not associated to an AP
            if(strcmp(token," (not associated) ") == 0){
                //pack empty BSSID if not associated
                char* temp = "00:00:00:00:00:00";
                PACK_BSSID(temp, &sbssid);
            }
            else{
                //load BSSID from CSV into struct
                PACK_BSSID(token+1, &sbssid);
            }
        }
        tokens++;
    } while( (token = strtok(NULL,",")) != NULL);
    
    printf("%15s", "Station BSSID: ");
    PRINT_BSSID(&bssid);
    printf("\n%15s", "AP BSSID: ");
    PRINT_BSSID(&sbssid);
    printf("\n\n");
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

    //DEBUG: print AP data
    printf("\nAP BSSID: ");
    PRINT_BSSID(&bssid);
    printf("\nAP ESSID (%lu): %s\n", strlen(essid), essid);
    printf("Channel: %d\n", channel);

    //create new AP entry
    AP ap = createAP(essid, bssid);
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
    //check if bssid already exists
    int j;
    for(j = 0; j < num_APs[i]; j++){
        if(memcmp(&(APs[i][j].bssid),&(ap.bssid), sizeof(struct BSSID)) == 0){
            printf("DUPLICATE FOUND\n");
            PRINT_BSSID(&(APs[i][j].bssid));
            printf("\n");
            PRINT_BSSID(&(ap.bssid));
            printf("\n");
            break;
        }
    }
    //if entry found...
    if(j != num_APs[i]){
        //if essids match prev record, erase new record
        if(strcmp(APs[i][j].essid,ap.essid) == 0)
            free(essid);
        //if new essid is not blank, overwrite old essid with new essid
        else{
            if(strlen(ap.essid) != 0){
                free(APs[i][j].essid);
                APs[i][j].essid = ap.essid;
            }
        }
        return;
    }

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
    return ap;
}
void addClient(AP ap, struct BSSID bssid){
    ap.num_clients ++;
    ap.clients = realloc(ap.clients, sizeof(BSSID) * ap.num_clients);
    ap.clients[ap.num_clients-1] = bssid;
}
void destroyAP(AP ap){
    free(ap.clients);
    free(ap.essid);
}
char** getCompatibleChannels(int* num_channels){
    //create an entry for every channel available
    FILE* commands = NULL;
    size_t buffer_sz = 255*sizeof(char);
    char* buffer = malloc(buffer_sz);
    memset(buffer, 0, buffer_sz);

    //create and execute commannd: iwlist {WIRELESS_DEVICE} channel
    char command[strlen(WIRELESS_DEVICE) + strlen("iwlist  channel 2>&1") + 1];
    strcpy(command , "iwlist ");
    strcat(command, WIRELESS_DEVICE);
    strcat(command, " channel 2>&1");
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
    char scan[strlen(WIRELESS_DEVICE) + strlen("   %d channels") + 1];
    strcpy(scan, WIRELESS_DEVICE);
    strcat(scan, "  %d channels");
    sscanf(buffer, scan, num_channels);
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
            printf("%-30s (", APs[c][a].essid);
            PRINT_BSSID(&(APs[c][a].bssid));
            printf("):\n");
            for(int client = 0; client < APs[c][a].num_clients; client++){
                printf("%10s","");
                PRINT_BSSID(&(APs[c][a].clients[client]));
            }
        }
    }
}