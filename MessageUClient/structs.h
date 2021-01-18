#pragma once

#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include "enums.h"

struct request{
    char clientId[uuid_length];
    unsigned char version_ = 1;
    unsigned char code;
    unsigned int size;
    std::string paylod;
};

struct response{
    unsigned char version;
    unsigned short int code;
    unsigned int size;
};

struct register_response {
    boost::uuids::uuid client_id;
};

struct clients_response {
    boost::uuids::uuid client_id;
    //char client_id[uuid_length];
    char client_name[user_max_length];
};

struct message_meta {
    unsigned int message_id;
    unsigned char Type;
    unsigned int size;
    //char* content;
};

struct server_info {
    std::string host;
    std::string port;
};

struct user_info {

    boost::uuids::uuid client_id;
};