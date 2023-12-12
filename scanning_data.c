#include "scanning_data.h"

extern AP** APs;
extern int* num_APs;
extern char** channels;
extern int num_channels;

void readCSV(char* fileName){
    //open up csv
    FILE* csv = fopen(fileName, "r");
    if(csv == NULL){
        printf("Error opening csv\n");
        exit(1);
    }
    //var holds length of data on a line
    int line_length = 0;
    //initialize buffer
    size_t buffer_sz = 255*sizeof(char);
    char* buffer = malloc(buffer_sz);

    //boolean to distinguish if we are on AP or station data
    bool reading_stations = false;

    //start reading csv line by line
    while( (line_length = getline(&buffer, &buffer_sz, csv)) != -1){
        //skip over blank lines
        if(line_length == 2) continue;

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
        else if(reading_stations) 
            read_Station_Data(buffer);  
    };

    free(buffer);
    fclose(csv);
}
void read_Station_Data(char* line){
    //var holds the column number
    int tokens = 0;
    //bssid of AP
    BSSID bssid;
    memset(&bssid, 0, sizeof(BSSID));
    //bssid of station
    BSSID sbssid;
    memset(&sbssid, 0, sizeof(BSSID));

    //tokenized line for CSV
    char* token = strtok(line,",");
    //iterate through different fields of CSV and extract relevant data
    do{
        //read in station BSSID
        if(tokens == 0)
            PACK_BSSID(token, &sbssid);
        //read in station's association
        else if(tokens == 5){
            //if station is not associated to an AP
            if(strcmp(token," (not associated) ") == 0){
                //pack empty BSSID if not associated
                PACK_BSSID("00:00:00:00:00:00", &bssid);
            }
            else{
                //load BSSID from CSV into struct
                PACK_BSSID(token+1, &bssid);
            }
        }
        tokens++;
    } while( (token = strtok(NULL,",")) != NULL);

    //find associated AP
    //iterate through channels
    for(int c = 0; c < num_channels; c++){
        //iterate through access points
        for(int a = 0; a < num_APs[c]; a++){
            //compare the BSSID of the AP the client is associated with to ones already in APs list
            if(memcmp(&(APs[c][a].bssid), &bssid, sizeof(BSSID)) == 0){
                //Once AP is found, see if client already exists in list
                for(int client = 0; client < APs[c][a].num_clients; client++){
                    if(memcmp( &APs[c][a].clients[client], &sbssid, sizeof(BSSID)) == 0){
                        return;
                    }
                }
                //if client not already in AP's list, add it
                addClient(&APs[c][a],sbssid);
                return;
            }
        }
    }
    //if AP not in list do nothing
}
void read_AP_Data(char* line){
    //BSSID of AP
    BSSID bssid;
    //ESSID of AP
    char* essid = NULL;
    //AP channel
    int channel = 0;
    //column number
    int tokens = 0;
    //tonized line
    char* token = strtok(line,",");
    //parse through fields of CSV data and extract relevant data
    do{
        //column 1 (bssid)
        if(tokens == 0) PACK_BSSID(token, &bssid);
        //column 4 (channel)
        else if(tokens == 3) sscanf(token, " %d", &channel);
        //column 14 (essid)
        else if(tokens == 13){

            if (strlen(token) == 1 && *token == ' ') essid=strdup(DEFAULT_ESSID);
            else essid = strdup(token+1);
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
        if(memcmp(&APs[i][a],&bssid, sizeof(BSSID)) == 0){
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
            if(strcmp(essid, DEFAULT_ESSID) != 0){
                free(APs[i][a].essid);
                APs[i][a].essid = essid;
            }
        }
        return;
    }

    //create new AP entry since it does not exist
    AP ap = createAP(essid, bssid, channel);

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