#include "collection.h"

#define WIRELESS_DEVICE "wlan0"
AP** APs;
int* num_APs;
char** channels;
int num_channels;

int main(int argc, char* argv[]){
    num_channels = 0;

    initVals();
    
    printf("Channels: %d\n\n", num_channels);
    for(int i = 0; i < num_channels; i++){
        printf("Channels: %s\n", channels[i]);
    }

    readCSV("channel1-01.csv");
    printReport();

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
    channels = getCompatibleChannels(&num_channels);

    if(num_channels == 0){
        printf("ERROR: WIRELESS DEVICE ERROR\n");
        exit(1);
    }

    APs = (AP**) malloc(sizeof(AP*) * num_channels);
    for(int i = 0; i < num_channels; i++){
        APs[i] = NULL;
    }
    num_APs = (int*) malloc(sizeof(int) * num_channels);
    for(int i = 0; i < num_channels; i++){
        num_APs[i] = 0;
    }
}
void readCSV(char* fileName){
    FILE* csv = fopen(fileName, "r");
    int line_length;
    size_t buffer_sz = 255*sizeof(char);
    char* buffer = malloc(buffer_sz);

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
            char* token = strtok(line,",");
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
    int tokens = 0;
    struct BSSID bssid;
    memset(&bssid, 0, sizeof(BSSID));
    struct BSSID sbssid;
    memset(&sbssid, 0, sizeof(BSSID));
    char* token = strtok(line,",");
    do{
        //read in station BSSID
        if(tokens == 0)
            PACK_BSSID(token, &bssid);
        //read in station's association
        else if(tokens == 5){
            if(strcmp(token," (not associated) ") != 0){
                PACK_BSSID(token+1, &sbssid);
            }
            //pack empty BSSID if not associated
            else{
                char* temp = "00:00:00:00:00:00";
                PACK_BSSID(temp, &sbssid);
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
    //if the line is not a blank or header line (contains AP data)
    struct BSSID bssid;
    char* essid = NULL;
    int channel = 0;
    int tokens = 0;
    char* token = strtok(line,",");
    do{
        if(tokens == 0)
            PACK_BSSID(token, &bssid);
        else if(tokens == 3){
            sscanf(token, " %d", &channel);
        }
        else if(tokens == 13){
            essid = strdup(token+1);
        }
        tokens++;
    } while( (token = strtok(NULL,",")) != NULL);

    printf("\nAP BSSID: ");
    PRINT_BSSID(&bssid);
    printf("\nAP ESSID (%lu): %s\n", strlen(essid), essid);
    printf("Channel: %d\n", channel);
    //create new AP entry
    AP ap = createAP(essid, bssid);

    //find out where this AP belongs
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
    FILE* commands;
    size_t buffer_sz = 255*sizeof(char);
    char* buffer = malloc(buffer_sz);

    //create and execute commannd
    char command[strlen(WIRELESS_DEVICE) + strlen("iwlist  channel") + 1];
    strcpy(command , "iwlist ");
    strcat(command, WIRELESS_DEVICE);
    strcat(command, " channel");
    commands = popen(command, "r");

    //printf("command: %s\n", command);
    getline(&buffer, &buffer_sz, commands);

    //printf("buffer: %s\n", buffer);
    char error_message[] = "no frequency information.";
    //printf("error: %s\n", error_message);

    if( strstr(buffer, error_message) == 0 ){
        pclose(commands);
        free(buffer);
        return NULL;
    }
    
    //scan for the channels compatible with our wireless device
    char scan[strlen(WIRELESS_DEVICE) + strlen("   %d channels") + 1];
    strcpy(scan, WIRELESS_DEVICE);
    strcat(scan, "  %d channels");
    sscanf(buffer, scan, num_channels);
    
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
    for(int c = 0; c < num_channels; c++){
        for(int a = 0; a < num_APs[c]; a++){
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