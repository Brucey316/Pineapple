# Pineapple

### Functionality
  This script uses a wireless card in monitor mode to scan available
  frequencies for Access Points (AP)s that have clients with them and
  utilizes WPA2 encryption. This script then performs deauth attacks
  on clients that are connected to these APs in order to sniff the WPA2
  handshake. The handshake is then stored locally and can be decrypted
  at a later time.
  
### Dependencies
- Airodump-ng
- Aircrack-ng Suite
- Wireless card that supports monitor mode
  
### Disclaimer
  DO NOT ATTACK ANY NETWORK THAT YOU DO NOT OWN. THIS IS STRICTLY PROOF OF CONCEPT
  AND ONLY TO BE RUN ON NETWORKS THAT YOU HAVE PERMISSION TO ATTACK. THIS IS AN
  INTRUSIVE ATTACK AS YOU ARE PERFORMING DoS ATTACKS.
