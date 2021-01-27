#pragma once

enum lengths {
    message_max_length = 1024,
    user_max_length = 255,
    uuid_length = 16,

    request_header_length = uuid_length + 1 + 1 + 4,

    // TODO: check why extra byte
    response_header_length = 1 + 2 + 4 + 1,
    max_clients = 100,
    client_list_max_bytes = (uuid_length + user_max_length) * max_clients
};


enum Action {
    Exit = 0,
    Register = 1,
    clients_list = 2,
    pkey = 3,
    get_messages= 4,
    send_message = 5,
    request_symetric_key = 51,
    send_symetric_key= 52
};

enum Request_Codes {
    registration_code = 100,
    client_list_code = 101,
    send_text_message_code = 103,
    get_messages_code = 104

};

enum Response_Codes {
    registration_success = 1000,
    client_list_success = 1001,
    message_sent_success = 1003,
    waiting_messages_success = 1004,
};