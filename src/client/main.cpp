#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string>
#include <unistd.h>

#include <linux/types.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <linux/rtnetlink.h>
#include <string.h>

#include "shared_data.h"
#include "netlink_send.hpp"
#include "netlink_listener.hpp"
#include "netlink_event.hpp"

using namespace std;

void usage() {
  fprintf(stdout, "-s   start with sending netlink request\n");
  fprintf(stdout, "-h   help message\n");
}

int main(int argc, char* const argv[]) {
  int ch;
  bool send_request = false;
  bool debug = false;
  
  cout << "netlink_main()" << endl;
  
  while ((ch = getopt(argc, argv, "sdh")) != -1) {
    switch (ch) {
    case 's':
      send_request = true;
      break;
    case 'd':
      debug = true;
      break;
    case 'h':
      usage();
      exit(0);
    }
  }  


  key_t shm_key;
  int shm_id;
  Sh_mem_data* shm_ptr;

  shm_key = ftok(".", 'a');
  shm_id = shmget(shm_key, sizeof(Sh_mem_data), 0666);
  if(shm_id < 0) {
    printf("ERROR: client could not get shared memory!\n");
    exit(1);
  }
  printf("client received shared memory!\n");

  shm_ptr = (Sh_mem_data*) shmat(shm_id, NULL, 0);
  if((long int)(shm_ptr) == -1) {
    printf("ERROR: client could not attach shared memory!\n");
    exit(1);
  }
  printf("client attached shared memory!\n");

  //clear destination memory
  bzero(shm_ptr->e_link_data, sizeof(shm_ptr->e_link_data));
  bzero(shm_ptr->e_addr_data, sizeof(shm_ptr->e_addr_data));
  bzero(shm_ptr->w_link_data, sizeof(shm_ptr->w_link_data));
  bzero(shm_ptr->w_addr_data, sizeof(shm_ptr->w_addr_data));

  strcpy(shm_ptr->e_link_data, "client says: no data received yet\n");
  strcpy(shm_ptr->e_addr_data, "client says: no data received yet\n");
  strcpy(shm_ptr->w_link_data, "client says: no data received yet\n");
  strcpy(shm_ptr->w_addr_data, "client says: no data received yet\n");


  cout << "will sleep for a while...\n";
  sleep(3);


  
  NetlinkSend nl_send;
  NetlinkListener nl_listener;

  int sock = nl_listener.init();
  if (sock <= 0) {
    cerr << "test_netlink(), bad voodoo. exiting.." << endl;
    exit(1);
  }

  if (send_request) {
    cout << "sending initial netlink request" << endl;
    nl_listener.set_multipart(true);
    if (nl_send.send(sock, RTM_GETLINK) != 0) {
      cerr << "test_netlink(), error sending" << endl;
      exit(1);
    }
  }

  while (true) {
    //    cout << "test_netlink: now entering listening mode: " << endl;

    NetlinkEvent nl_event;
    if (nl_listener.process(nl_event) == true) {
      if (send_request) {
        if (nl_send.send(sock, RTM_GETADDR) != 0) {
          cerr << "test_netlink(), error sending" << endl;
          exit(1);
        }
        send_request = false;
      }
      else {
        nl_listener.set_multipart(false);
      }

      stringstream data_stream;

      char buf[20];
      sprintf(buf, "%d", nl_event.get_index());
      data_stream << "results for " << nl_event.get_iface() << 
        "(" << string(buf) << ")" << endl;
      data_stream << "  running: " << 
        string(nl_event.get_running() ? "yes" : "no") << endl;
      data_stream << "  enabled: " << 
        string(nl_event.get_enabled() ? "yes" : "no") << endl;
      if (debug) {
        cout << "  ifinfomsg: " << nl_event.get_ifinfomsg() << endl;
      }
      if (nl_event.get_type() == RTM_DELLINK || 
   nl_event.get_type() == RTM_NEWLINK) {
        data_stream << "  type: " << 
          string(nl_event.get_type()==RTM_DELLINK?"DELLINK":"NEWLINK") << endl;
        data_stream << "  state: " << string(nl_event.is_link_up()?"UP":"DOWN") << endl;
        sprintf(buf, "%d", nl_event.get_mtu());
        data_stream << "  mtu: " << string(buf) << endl;
        data_stream << "  mac: " << nl_event.get_mac_str() << endl;

        //todo research linux link interface naming schemes more, there is a chance this might lead to surprises
        if(nl_event.get_iface()[0] == 'e') {
            strcpy(shm_ptr->e_link_data, data_stream.str().c_str());
        }
        else if(nl_event.get_iface()[0] == 'w') {
            strcpy(shm_ptr->w_link_data, data_stream.str().c_str());
        }
      }
      else if (nl_event.get_type() == RTM_DELADDR ||
        nl_event.get_type() == RTM_NEWADDR) {
        data_stream << "  type: " << 
          string(nl_event.get_type()==RTM_DELADDR?"DELADDR":"NEWADDR") << endl;
        data_stream << "  addr: " << nl_event.get_addr().str().c_str() << endl;
        data_stream << "  broadcast: " << nl_event.get_broadcast().str().c_str() << endl;
        char buf[20];
        sprintf(buf, "%d", nl_event.get_mask_len());
        data_stream << "  mask length: " << string(buf) << endl;

        if(nl_event.get_iface()[0] == 'e') {
            strcpy(shm_ptr->e_addr_data, data_stream.str().c_str());
        }
        else if(nl_event.get_iface()[0] == 'w') {
            strcpy(shm_ptr->w_addr_data, data_stream.str().c_str());
        }
      }
      data_stream << endl;
      cout << data_stream.str();

    }
    else {
      //      cout << "didn't receive a message, sleeping for 1 second" << endl;
      sleep(1);
    }
  }

  //detach shared memory
  shmdt((void *) shm_ptr);
  cout << "client detached shared memory\n";
  //remove shared memory
  cout << "client removed shared memory\n";
  shmctl(shm_id, IPC_RMID, NULL);


  cout << "client exiting\n";
  exit(0);
}
