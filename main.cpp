/* 
 * File:   main.cpp
 * Author: vitex@hippy.cz
 *
 * Created on 6. listopad 2013, 12:16
 */

#include "james-redirector.hxx"
#include <iostream>
#include <string>
#include <stdlib.h>
#include <libconfig.h++>
#include <mysql++/mysql++.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;
using namespace libconfig;
// a replace function :)

bool replace(string& str, const string& from, const string& to) {
    size_t start_pos = str.find(from);
    if (start_pos == string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

/**
 * Rozhoduje zdali se jedna o request na apple.com/success.html
 */
bool isImage(string url) {
    if (url.find("image?url=") != string::npos)
        return true;
    if (url.find(".png") != string::npos)
        return true;
    if (url.find(".jpeg") != string::npos)
        return true;
    if (url.find(".gif") != string::npos)
        return true;
    if (url.find(".jpg") != string::npos)
        return true;
    if (url.find(".PNG") != string::npos)
        return true;
    if (url.find(".JPEG") != string::npos)
        return true;
    if (url.find(".GIF") != string::npos)
        return true;
    if (url.find(".JPG") != string::npos)
        return true;

    return false;
}

string getProtocol(string url) {
    int pr = url.find("://");
    if (pr == string::npos) {
        return "http";
    }
    return url.substr(0, pr);
}

string getDomain(string url) {
    int protocolPosition = url.find("://");
    string tmp = url;
    if (protocolPosition != string::npos) {
        tmp = url.substr(protocolPosition + 3, url.length());
    }

    int slashPosition = tmp.find("/");

    if (slashPosition != string::npos) {
        tmp = tmp.substr(0, slashPosition);
    }
    return tmp;
}

string getServerIP(string interface) {
    string address;
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    /* Walk through linked list, maintaining head pointer so we
       can free list later */

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        if (!strcmp(ifa->ifa_name, interface.c_str()))
            if (family == AF_INET /*|| family == AF_INET6 */) {
                s = getnameinfo(ifa->ifa_addr,
                        (family == AF_INET) ? sizeof (struct sockaddr_in) :
                        sizeof (struct sockaddr_in6),
                        host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
                if (s != 0) {
                    printf("getnameinfo() failed: %s\n", gai_strerror(s));
                    exit(EXIT_FAILURE);
                }
                address.assign( host) ;
            }
    }

    freeifaddrs(ifaddr);


    return address;
}

int main() {
    Config cfg;
    mysqlpp::Connection sqlConn;
    string landingPage, landingPageDomain, cacheUrl, dbname, dbhost, dblogin, dbpassw, protocol, interface, serverIP;

    try {
        cfg.readFile("/etc/james.conf");
    } catch (const FileIOException &fioex) {
        cerr << "File I/O error" << endl;
        return (EXIT_FAILURE);
    } catch (const ParseException &pex) {
        cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
                << " - " << pex.getError() << endl;
        return (EXIT_FAILURE);
    }

    try {
        string cfgdbname = cfg.lookup("dbname");
        dbname.assign(cfgdbname);
    } catch (const SettingNotFoundException &nfex) {
        cerr << "No 'dbname' setting in configuration file." << endl;
    }

    try {
        string cfgdbhost = cfg.lookup("dbhost");
        dbhost.assign(cfgdbhost);
    } catch (const SettingNotFoundException &nfex) {
        cerr << "No 'dbhost' setting in configuration file." << endl;
    }

    try {
        string cfgdblogin = cfg.lookup("dblogin");
        dblogin.assign(cfgdblogin);
    } catch (const SettingNotFoundException &nfex) {
        cerr << "No 'dblogin' setting in configuration file." << endl;
    }

    try {
        string cfgdbpassw = cfg.lookup("dbpassw");
        dbpassw.assign(cfgdbpassw);
    } catch (const SettingNotFoundException &nfex) {
        cerr << "No 'dbpassw' setting in configuration file." << endl;
    }

    try {
        string cfgcacheurl = cfg.lookup("cachepath");
        cacheUrl.assign(cfgcacheurl);
    } catch (const SettingNotFoundException &nfex) {
        cerr << "No 'cacheurl' setting in configuration file." << endl;
    }

    try {
        string cfginterface = cfg.lookup("interface");
        interface.assign(cfginterface);
    } catch (const SettingNotFoundException &nfex) {
        cerr << "No 'interface' setting in configuration file." << endl;
    }

    if (sqlConn.connect(dbname.c_str(), dbhost.c_str(), dblogin.c_str(), dbpassw.c_str())) {

        string options_query = "SELECT * FROM `options` WHERE `key`='landingpage'";
        mysqlpp::Query query = sqlConn.query(options_query);
        mysqlpp::StoreQueryResult res = query.store(); //Problem

        if (res) {
            mysqlpp::Row row;
            string keyword;
            string value;
            mysqlpp::StoreQueryResult::const_iterator it;
            for (it = res.begin(); it != res.end(); ++it) {
                row = *it;
                keyword.assign(row[0]);
                value.assign(row[1]);
                if (keyword == "landingpage") {
                    landingPage = value;
                }
            }

        } else {
            cerr << "Failed to load options: " << query.error() << endl;
        }

        sqlConn.disconnect();

        landingPageDomain = getDomain(landingPage);
        serverIP = getServerIP(interface);


        size_t start_pos;
        string input;

        while (1) {
            cin >> input;
            start_pos = input.find(landingPageDomain);
            if ((start_pos != string::npos) && isImage(input)) {
                protocol = getProtocol(input);
                input = protocol.append("://").append(serverIP).append(cacheUrl).append(input.substr(input.find(landingPageDomain) + landingPageDomain.length() + 1));
            }
            cout << input << endl;
        }

    } else {
        cerr << "DB connection failed: " << sqlConn.error() << endl;

    }

    return (EXIT_SUCCESS);
}