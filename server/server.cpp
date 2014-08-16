// Copyright (c) 2014 The PageDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors

#include "Socket.h"
#include "Acceptor.h"
#include "EventPool.h"
#include "EventLoop.h"
#include "MSGHandler.h"

#include "Option.h"
#include "PageDB.h"
#include "BufferPacket.h"
using namespace pagedb;

#define BASE_PORT 10000
#define PORT_NUM  1

EventPool server(4);
PageDB   *db;

class Parser
{
public:
    Parser(const Buffer &buff) : status_(false)
    {
        doParse(buff);
    }

    bool   status() const { return status_; }
    string type()   const { return type_;   }

    map<string, string> values() const { return values_; }

private:
    void doParse(const string &str)
    {
        if(str.size() <= 2) 
        {
            status_ = false;
            return;
        }
        int i = 0;
        while(i < str.size() && str[i] == ' ') i++;
        if(i == str.size() || str[i] != '{')
        {
            status_ = false;
            return;
        }

        i++;
        while(i < str.size())
        {
            while(i < str.size() && str[i] != '\"') i++;
            if(i == str.size()) break;
            int j = i + 1;
            while(j < str.size() && str[j] != '\"') j++;
            string key = str.substr(i + 1, j - i - 1);
            i = j + 1;
            while(i < str.size() && str[i] != '\"') i++;
            j = i + 1;
            while(j < str.size() && str[j] != '\"') j++;
            string val = str.substr(i + 1, j - i - 1);
            values_[key] = val;
            i = j + 1;
        }
    }

private:
    string              type_;
    map<string, string> values_;
};


class DBServer : public eventserver::MSGHandler
{
public:
    DBServer(eventserver::EventLoop *loop, eventserver::Socket sock) :\
        eventserver::MSGHandler(loop, sock, 1) 
    { 
    }
private:
    void received(eventserver::STATUS status, eventserver::Buffer &buff)
    {  
        Parser parser(buff);
        assert(buff.status() == true);
        string type = parser.type();
        if(type == "write")
        {
            map<string, string> values = parser.values();
            if(values.find("key") != values.end() && values.find("value") != values.end())
            {
                bool flag = db -> put(values["key"], values["value"]);
                if(flag == true)
                {
                    write("{\"status\" : \"successful\"}");
                }
                else
                {
                    write("{\"status\" : \"failure\"}");
                }
            }
        }
        else if(type == "read")
        {
            map<string, string> values = parser.values();
            if(values.find("key") != values.end())
            {
                string value = db -> get(values["key"]);
                if(values.size() == 0)
                {
                    write("{\"status\" : \"failure\"}");
                }
                else
                {
                    string str = "{\"status\" : \"successful\"";
                    str += ", \"value\" : \"" + value + "\"}";
                    write(str); 
                }
            }
            else
            {
                write("{\"status\" : \"failure\"}");
            }
        }
        else
        {
            write("{\"status\" : \"failure\"}");
        }
    }
};

void signalStop(int) { pool.stop(); }

int setlimit(int num_pipes)
{
    struct rlimit rl;
    rl.rlim_cur = rl.rlim_max = num_pipes * 2 + 50;
    if (::setrlimit(RLIMIT_NOFILE, &rl) == -1)
    {
        fprintf(stderr, "setrlimit error: %s", strerror(errno));
        return 1;
    }
}

void init_config()
{
	setlimit(100000);
	errno = 0;
}

void init_db()
{
    Options option;
    option.logOption.disabled = true;
    option.logOption.logLevel = Log::LOG_FATAL;

    db = new PageDB();
    
    db -> open(option);
}

void init_server()
{
	Log::setLevel(Log::warn);
	
	vector<TCPAcceptor<DBServer>*> acceptors(PORT_NUM, NULL);
	
	for(int i = 0; i < PORT_NUM; i++)
        acceptors[i] = new TCPAcceptor<DBServer>(server.loop(), BASE_PORT + i);
   	
   	server.attach(SIGINT, signalStop);

    server.run();
}

int main()
{
	init_config();
	init_db();
	init_server();

    return 0;
}