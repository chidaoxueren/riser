//
//  gent_connect.cpp
//  riser
//
//  Created by wyt on 13-1-27.
//  Copyright (c) 2013年 wyt. All rights reserved.
//

#include "gent_connect.h"
#include "gent_event.h"
#include "gent_level.h"

#include <errno.h>
GentConnect::GentConnect(int sfd)
{
	Init(sfd);
}

GentConnect::~GentConnect()
{
    Destruct();
}
void GentConnect::Destruct()
{
    if(!fd) return;
    close(fd);
	LOG(GentLog::INFO, "file description %d close.", fd);
    if(rbuf) {
        free(rbuf);
    }
    GentAppMgr::Instance()->Destroy(fd);
}
void GentConnect::Init(int sfd) {
    fd = sfd;
    clen = 0;
    remainsize = 0;
    //configure info
    comm = GentAppMgr::Instance()->GetCommand(this, fd);
    ReAllocation();
}

void GentConnect::ReAllocation() {
    curstatus = Status::CONN_READ;
    rsize = GentCommand::READ_BUFFER_SIZE;
	rcurr = NULL;
    content = NULL;
	actualsize = 0;
    rbuf = (char *)malloc(rsize);
    memset(rbuf,0,rsize);
}

void GentConnect::Reset() {
    comm->Reset();
    if(rbuf) {
        free(rbuf);
    }
    if(content) {
        free(content);
    }
    ReAllocation();
}

int GentConnect::TryRunning(string &outstr2) {
    char *new_rbuf;
    int rbytes = 0;
    int readNum;
    int stop=0;
	while(true) {
	  if(stop==1) break;
        switch(curstatus) {
            case Status::CONN_READ:
                outstr = "";
                readNum = InitRead(rbytes);
                LOG(GentLog::INFO, "init read the number of byte is %d.", readNum);
                if(readNum < 0) {
                    LOG(GentLog::WARN, "init read the number of byte less than zero");
                    outstr2 = "read error\r\n";
                    Reset();
                    return readNum;
                }else if(readNum == 0) {                                
                    return readNum;
                }
                remainsize = comm->Process(rbuf, rbytes, outstr);
                LOG(GentLog::INFO, "curstatus: %d",curstatus);
                 if(!remainsize && outstr != "") {
                     curstatus = Status::CONN_WRITE;
                     gevent->UpdateEvent(fd, this, eventWrite);
                }
                break;
            case Status::CONN_NREAD:
                //OutString("ok\r\n"); 
                //LOG(GentLog::INFO, "start conn_nread remainsize:%d",remainsize);
				if(!content) {
                    new_rbuf = (char *)malloc(remainsize);
                    memset(new_rbuf,0,remainsize);
                    rcont = content = new_rbuf;
                }
                if(NextRead() == -1) {
                    stop = 1;
                    break;		
                }
                break;
            case Status::CONN_DATA:
                outstr = "";
                comm->Complete(outstr,content, actualsize);
                curstatus = Status::CONN_WRITE;
                gevent->UpdateEvent(fd, this, eventWrite);
                
                return 0;
            case Status::CONN_WRITE:
                OutString(outstr);
                break;
            case Status::CONN_WAIT:
				LOG(GentLog::INFO, "the status of %d is connect wait", fd);
                remainsize = 0;
                Reset();
                curstatus = Status::CONN_READ;
                //GentEvent::Instance()->UpdateEvent(fd, this);
                gevent->UpdateEvent(fd, this, eventRead);
                
                return 0;
            case Status::CONN_CLOSE:
                return -1;		
            default:
                return -1;
        }
        
	}
	return 1;
}

int GentConnect::InitRead(int &rbytes) {                         
    int gotdata = 0;                                                     
    int res;                                                             
    int num_allocs = 0;                                                  
 	                                                                      
    while (1) {                                                          
        if (rbytes >= rsize) {
            if (num_allocs == 4) {                                       
                return gotdata;                                          
            }                                                            
            ++num_allocs;                                                
            char *new_rbuf = (char *)realloc(rbuf, rsize * 2);
            if (!new_rbuf) {                                             
                LOG(GentLog::ERROR, "Couldn't realloc input buffer");
                rbytes = 0; /* ignore what we read */
                return -2;                                               
            } 
            rbuf = new_rbuf;                               
            rsize *= 2;
        }                                                                
                                                                         
        int avail = rsize - rbytes;
        res = read(fd, rbuf + rbytes, avail);                   
		if (res > 0) {                                                   
            gotdata = 1;                                                 
            rbytes += res;
            if (res == avail) {                                          
                continue;                                                
            } else {                                                     
                break;                                                   
            }                                                            
        }                                                                
       if (res == 0) {
           return -3;                                       
       }                                                    
       if (res == -1) {
           if (errno == EAGAIN || errno == EWOULDBLOCK) {   
               break;                                       
           }
           return -1;
       }                                                    
   }                                                        
   return gotdata;                                          
}

void GentConnect::OutString(const string &str) {
   /*
    struct msghdr msg;
    struct iovec iov[1];
    bzero(&msg, sizeof(msg));
    
    msg.msg_name = &request_addr;
    msg.msg_namelen = sizeof(request_addr);
    char buf2[20]={0};
    memcpy(buf2,str.c_str(),str.size());
    iov[0].iov_base = buf2;
    iov[0].iov_len = str.size();
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    int a = sendmsg(fd,&msg,0);
    */
    //int a = write(fd, str.c_str(),str.size());
    uint64_t length = str.size();
	uint32_t curlen = 0;
	char *curpos = const_cast<char *>(str.c_str());	
	int slen;
	while(length>0) {
    	slen = send(fd, curpos+curlen, length, 0);
		if(slen<0) break;
		//cout << "write: " << slen << " : " << str.size() << " length: "<< length << endl;
		length -= slen;
		curlen += slen;
	}
    curstatus = Status::CONN_WAIT;
}

void GentConnect::SetStatus(int s) {
    curstatus = s;
}

int GentConnect::NextRead() {
	if (remainsize <= 0) {
		SetStatus(Status::CONN_DATA);
		return 0;		
	}
                                                              
	int res;
	/*  now try reading from the socket */                           
	res = read(fd, rcont, remainsize);
	if (res > 0) {
		rcont += res;                                             
		remainsize -= res;
		actualsize += res;                                           
		return 0;                                  
	}                                                                
	if (res == 0) { /* end of stream */                             
	    SetStatus(Status::CONN_CLOSE);
	    return 0;
	}                                                               
	if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {   
        gevent->UpdateEvent(fd, this, eventRead);
		return -1;                                                            
	}
	SetStatus(Status::CONN_CLOSE);                                                               
	return 0;
}
