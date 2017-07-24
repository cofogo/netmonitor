#include <errno.h>
#include <linux/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/rtnetlink.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <iostream>

#include "netlink_event.hpp"
#include "netlink_listener.hpp"


using namespace std;


#define SO_RCV_BUF_SIZE_MAX      (256*1024) // Desired socket buffer size
#define SO_RCV_BUF_SIZE_MIN      (48*1024)  // Min. socket buffer size
#define NLSOCK_BYTES  (8*1024)

/**
 *
 *
 **/
NetlinkListener::NetlinkListener() : 
  _fd(-1),
  _is_multipart_message_read(false)
{

}

/**
 *
 *
 **/
NetlinkListener::~NetlinkListener()
{
  close(_fd);
}

/**
 *
 *
 **/
int
NetlinkListener::init()
{
  struct sockaddr_nl snl;
  socklen_t  snl_len;
  
  if (_fd >= 0) {
    cerr << "socket cannot be initialized" << endl;
    return _fd;
  }
  
  _fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
  if (_fd < 0) {
    cerr << string("Could not open netlink socket: ") <<  strerror(errno) << endl;
    return _fd;
  }
  
  comm_sock_set_rcvbuf(_fd, SO_RCV_BUF_SIZE_MAX, SO_RCV_BUF_SIZE_MIN);
  
  memset(&snl, 0, sizeof(snl));
  snl.nl_family = AF_NETLINK;
  snl.nl_pid    = getpid();  // Let the kernel assign the pid to the socket
  snl.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR;//_nl_groups;
  if (bind(_fd, reinterpret_cast<sockaddr*>(&snl), sizeof(snl)) < 0) {
    cerr << string("bind(AF_NETLINK) failed: ") << strerror(errno) << endl;
    close(_fd);
    _fd = -1;
    return _fd;
  }
  
  snl_len = sizeof(snl);
  if (getsockname(_fd, reinterpret_cast<sockaddr*>(&snl), &snl_len) < 0) {
    cerr << string("getsockname(AF_NETLINK) failed: ") << strerror(errno) << endl;
    close(_fd);
    _fd = -1;
    return _fd;
  }
  if (snl_len != sizeof(snl)) {
    cerr << string("Wrong address length of AF_NETLINK socket: ") << endl;
    close(_fd);
    _fd = -1;
    return _fd;
  }
  if (snl.nl_family != AF_NETLINK) {
    cerr << string("Wrong address family of AF_NETLINK socket: ") << endl;
    close(_fd);
    _fd = -1;
    return _fd;
  }
  return _fd;
}

/**
 *
 *
 **/
bool
NetlinkListener::process(NetlinkEvent &e)
{
  if (_fd <= 0) {
    return false;
  }

  vector<char> message;
  vector<char> buffer(NLSOCK_BYTES);
  size_t last_mh_off = 0;
  size_t off = 0;
  ssize_t got = -1;

    char buf[20];

  for ( ; ; ) {
    //don't block on recv
    do {
      got = recv(_fd, &buffer[0], buffer.size(), MSG_DONTWAIT | MSG_PEEK);
      if ((got < 0) && (errno == EINTR))
 continue; // XXX: the receive was interrupted by a signal
      if ((got < 0) || (got < (ssize_t)buffer.size()))
 break;  // The buffer is big enough
      buffer.resize(buffer.size() + NLSOCK_BYTES);
    } while (true);
    
    got = recv(_fd, &buffer[0], buffer.size(),
        MSG_DONTWAIT);
    //    got = read(_fd, &buffer[0], buffer.size());
    if (got < 0) {
      if (errno == EINTR)
 continue;
      //      cerr << "Netlink socket read error: " << endl;
      break;
    }

    message.resize(message.size() + got);
    memcpy(&message[off], &buffer[0], got);
    off += got;
    
    if ((off - last_mh_off) < (ssize_t)sizeof(struct nlmsghdr)) {
      cerr << string("Netlink socket recvfrom failed: message truncated: ") << endl;
      break;
    }
    
    //
    // If this is a multipart message, it must be terminated by NLMSG_DONE
    //

    bool is_end_of_message = false;
    size_t new_size = off - last_mh_off;
    const struct nlmsghdr* mh;
    for (mh = reinterpret_cast<nlmsghdr*>(&buffer[last_mh_off]);
  NLMSG_OK(mh, new_size);
  mh = NLMSG_NEXT(const_cast<nlmsghdr*>(mh), new_size)) {
      if ((mh->nlmsg_flags & NLM_F_MULTI)
   || _is_multipart_message_read) {
 sprintf(buf, "%d", mh->nlmsg_type);
 is_end_of_message = false;
 if (mh->nlmsg_type == NLMSG_DONE) {
   is_end_of_message = true;
 }
      }
    }
    last_mh_off = reinterpret_cast<size_t>(mh) - reinterpret_cast<size_t>(&buffer[0]);
    if (is_end_of_message) {
      break;
    }
  }

  _nl_event_mgr.process(reinterpret_cast<unsigned char*>(&message[0]), off);
  return _nl_event_mgr.pop(e);
}

/**
 *
 *
 **/
int
NetlinkListener::comm_sock_set_rcvbuf(int sock, int desired_bufsize, int min_bufsize)
{
    int delta = desired_bufsize / 2;

    /*
     * Set the socket buffer size.  If we can't set it as large as we
     * want, search around to try to find the highest acceptable
     * value.  The highest acceptable value being smaller than
     * minsize is a fatal error.
     */
    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF,
     &desired_bufsize,
     sizeof(desired_bufsize)) < 0) {
 desired_bufsize -= delta;
 while (1) {
     if (delta > 1)
  delta /= 2;

     if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF,
      &desired_bufsize,
      sizeof(desired_bufsize)) < 0) {
      desired_bufsize -= delta;
      if (desired_bufsize <= 0)
        break;
      } 
      else {
        if (delta < 1024) {
          break;
        }
        desired_bufsize += delta;
     }
 }
 if (desired_bufsize < min_bufsize) {
   cerr << "Cannot set receiving buffer size of socket" << endl;
   return -1;
 }
    }

    return (desired_bufsize);
}
