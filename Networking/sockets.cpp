
#include "sockets.h"
#include "Tools/Exceptions.h"
#include "Tools/time-func.h"

#include <iostream>
#include <fcntl.h>
using namespace std;

void error(const char *str)
{
  char err[1000];
  gethostname(err,1000);
  strcat(err," : ");
  strcat(err,str);
  perror(err);
  throw bad_value();
}

void error(const char *str1,const char *str2)
{
  char err[1000];
  gethostname(err,1000);
  strcat(err," : ");
  strcat(err,str1);
  strcat(err,str2);
  perror(err);
  throw bad_value();
}

void set_up_client_socket(int& mysocket,const char* hostname,int Portnum)
{
   mysocket = socket(AF_INET, SOCK_STREAM, 0);
   if (mysocket<0) { error("set_up_socket:socket");  }

  /* disable Nagle's algorithm */
  int one=1;
  int fl= setsockopt(mysocket, IPPROTO_TCP, TCP_NODELAY, (char*)&one, sizeof(int));
  if (fl<0) { error("set_up_socket:setsockopt");  }

  fl=setsockopt(mysocket, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(int));
  if (fl<0) { error("set_up_socket:setsockopt"); }

   struct addrinfo hints, *ai=NULL,*rp;
   memset (&hints, 0, sizeof(hints));
   hints.ai_family = AF_INET;
   hints.ai_flags = AI_CANONNAME;

   octet my_name[512];
   memset(my_name,0,512*sizeof(octet));
   gethostname((char*)my_name,512);

   int erp;
   for (int i = 0; i < 60; i++)
     { erp=getaddrinfo (hostname, NULL, &hints, &ai);
       if (erp == 0)
         { break; }
       else
         { cerr << "getaddrinfo on " << my_name << " has returned '" << gai_strerror(erp) <<
           "' for " << hostname << ", trying again in a second ..." << endl;
           if (ai)
             freeaddrinfo(ai);
           sleep(1);
         }
     }
   if (erp!=0)
     { error("set_up_socket:getaddrinfo");  }

   bool success = false;
   socklen_t len = 0;
   const struct sockaddr* addr = 0;
   for (rp=ai; rp!=NULL; rp=rp->ai_next)
      { addr = ai->ai_addr;

        if (ai->ai_family == AF_INET)
           {
             len = ai->ai_addrlen;
             success = true;
             continue;
           }
      }

   if (not success)
     {
       for (rp = ai; rp != NULL; rp = rp->ai_next)
         cerr << "Family on offer: " << ai->ai_family << endl;
       runtime_error(string("No AF_INET for ") + (char*)hostname + " on " + (char*)my_name);
     }


   Timer timer;
   timer.start();
   struct sockaddr_in* addr4 = (sockaddr_in*) addr;
   addr4->sin_port = htons(Portnum);      // set destination port number
#ifdef DEBUG_IPV4
   cout << "connect to ip " << hex << addr4->sin_addr.s_addr << " port " << addr4->sin_port << dec << endl;
#endif

   int attempts = 0;
   long wait = 1;
   do
   {  fl=1;
      while (fl==1 || errno==EINPROGRESS)
        {
          fl=connect(mysocket, addr, len);
          attempts++;
          if (fl != 0)
            usleep(wait *= 2);
        }
   }
   while (fl == -1 && (errno == ECONNREFUSED || errno == ETIMEDOUT)
            && timer.elapsed() < 60);

   if (fl < 0)
     {
       cout << attempts << " attempts" << endl;
       error("set_up_socket:connect:", hostname);
     }

   freeaddrinfo(ai);

#ifdef __APPLE__
  int flags = fcntl(mysocket, F_GETFL, 0);
  fl = fcntl(mysocket, F_SETFL, O_NONBLOCK |  flags);
  if (fl < 0)
    error("set non-blocking");
#endif
}

void close_client_socket(int socket)
{
  if (close(socket))
    {
      char tmp[1000];
      sprintf(tmp, "close(%d)", socket);
      error(tmp);
    }
}

unsigned long long sent_amount = 0, sent_counter = 0;
