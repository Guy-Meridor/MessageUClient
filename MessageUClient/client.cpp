#include <iostream>
#include <filesystem>
#include <string>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include "enums.h"
#include "structs.h"




#define USER_PATH "me.info"
#define SERVER_PATH "server.info"

using namespace std;
using boost::asio::ip::tcp;




server_info get_server_info_from_file() {
	server_info info = { "localhost", "1234" };
	ifstream server_info_file(SERVER_PATH);

	if (server_info_file.is_open())
	{
		if (server_info_file.good())
		{
			// Get host
			getline(server_info_file, info.host, ':');

			if (server_info_file.good())
			{
				// Get port
				getline(server_info_file, info.port, ':');
			}
		}

		server_info_file.close();
	}

	return info;

}

void get_clients(tcp::socket* s, boost::uuids::uuid uuid, std::map<boost::uuids::uuid, std::string>& usernames) {
	std::vector<boost::asio::const_buffer> buffers;
	request request;
	response response;
	clients_response client;
	usernames.clear();

	std::copy(uuid.begin(), uuid.end(), request.clientId);
	request.code = client_list_code;
	request.size = 0;

	buffers.push_back(boost::asio::buffer(&request.clientId, uuid_length));
	buffers.push_back(boost::asio::buffer(&request.version_, 1));
	buffers.push_back(boost::asio::buffer(&request.code, 1));
	buffers.push_back(boost::asio::buffer(&request.size, 4));
	boost::asio::write(*s, buffers);

	boost::asio::read(*s, boost::asio::buffer(&response, response_header_length));
	if (response.code == client_list_success && response.size > 0) {
		size_t num_of_clients = response.size / sizeof clients_response;

		for (size_t i = 0; i < num_of_clients; i++)
		{
			boost::asio::read(*s, boost::asio::buffer(&client, sizeof client));
			std::string username(client.client_name);
			usernames[client.client_id] = username;
		}
	}

}

boost::uuids::uuid get_user_uid(std::string username, std::map<boost::uuids::uuid, std::string>& usernames) {

	for (const auto& [uid, uname] : usernames) {
		if (username.compare(uname) == 0) {
			return uid;
		}
	}

	return boost::uuids::nil_uuid();
}

int main(int argc, char* argv[])
{
	try
	{
		// socket connection
		server_info info = get_server_info_from_file();
		boost::asio::io_context io;
		tcp::socket s(io);
		tcp::resolver resolver(io);
		boost::asio::connect(s, resolver.resolve(info.host.c_str(), info.port.c_str()));

		// header 
		char username[user_max_length];
		char* uid = new char[uuid_length];


		// helpers
		string strUsername;
		string strUuid;

		// register
		register_response reg_res;
		boost::uuids::uuid uuid;
		string user_action;
		boost::filesystem::path user_file(USER_PATH);

		// get clients
		//clients_response clients[max_clients];
		std::map<boost::uuids::uuid, std::string> usernames;
		size_t num_of_clients;

		//get messages
		message_meta msg_meta;
		char* msg_content;

		// send text message
		boost::uuids::uuid rec_uid;
		std::string str_rec_username;
		char message[message_max_length];
		send_message_payload_meta send_message_request_meta;
		message_sent msg_sent_response;



		// user file
		ifstream user_input_stream;
		ofstream user_output_stream;

		request request;
		size_t request_length;
		response response;
		std::vector<boost::asio::const_buffer> buffers;

		// Get instructions from user
		while (true) {
			memset(username, 0, sizeof username);
			memset(message, 0, sizeof message);
			memset(&request, 0, sizeof request);
			memset(&response, 0, sizeof response);
			buffers.clear();
			user_action.clear();
			request.version_ = 1;

			try
			{
				cout << "------------------------------------" << std::endl;
				if (!strUsername.empty())
				{
					std::cout << "Hello " << strUsername << std::endl;
				}

				std::cout << "MessageU client at your service." << std::endl <<
					Register << ". Register" << std::endl <<
					clients_list << ". Request for clients list" << std::endl <<
					pkey << ". Request for public key" << std::endl <<
					get_messages << ". Request for waiting messages" << std::endl <<
					send_message << ". Send a text message" << std::endl <<
					request_symetric_key << ". Send a request for symmetric key" << std::endl <<
					send_symetric_key << ". Send your semetric key" << std::endl <<
					Exit << ". Exit client" << std::endl;

				std::getline(std::cin, user_action);

				switch (std::stoi(user_action))
				{
				case Exit:
					return 0;
					break;

				case Register:
					if (boost::filesystem::exists(user_file)) {
						std::cout << "user is already registered, getting user data." << std::endl;

						user_input_stream.open("me.info");
						std::getline(user_input_stream, strUsername);
						std::getline(user_input_stream, strUuid);
						uuid = boost::lexical_cast<boost::uuids::uuid>(strUuid);
						user_input_stream.close();
						break;
					}

					std::cout << "Enter username: ";
					std::cin.getline(username, user_max_length);
					

					// Send to server
					request.code = registration_code;
					request.size = strlen(username);
					std::copy(uuid.begin(), uuid.end(), request.clientId);
					buffers.push_back(boost::asio::buffer(&request.clientId, uuid_length));
					buffers.push_back(boost::asio::buffer(&request.version_, 1));
					buffers.push_back(boost::asio::buffer(&request.code, 1));
					buffers.push_back(boost::asio::buffer(&request.size, 4));
					buffers.push_back(boost::asio::buffer(username, strlen(username)));
					boost::asio::write(s, buffers);


					// TODO: check why extra byte
					boost::asio::read(s, boost::asio::buffer(&response, response_header_length));

					if (response.code == registration_success) {
						strUsername = std::string(username);
						boost::asio::read(s, boost::asio::buffer(&reg_res, uuid_length));
						uuid = reg_res.client_id;
						strUuid = boost::uuids::to_string(uuid);
						user_output_stream.open("me.info");
						user_output_stream << username << std::endl << strUuid;
						user_output_stream.close();
					}

					break;

				case clients_list:
					if (!boost::filesystem::exists(user_file)) {
						std::cout << "Action requires registration." << std::endl;
						break;
					}

					get_clients(&s, uuid, usernames);

					for (const auto& [uid, uname] : usernames) {
						std::cout << uid << " - " << uname << std::endl;
					}

					break;

				case pkey:
					if (!boost::filesystem::exists(user_file)) {
						std::cout << "Action requires registration." << std::endl;
						break;
					}
					request.code = 102;

					break;

				case get_messages:
					if (!boost::filesystem::exists(user_file)) {
						std::cout << "Action requires registration." << std::endl;
						break;
					}

					get_clients(&s, uuid, usernames);
					std::copy(uuid.begin(), uuid.end(), request.clientId);
					request.code = get_messages_code;
					request.size = 0;
					request.version_ = 1;

					buffers.push_back(boost::asio::buffer(&request.clientId, uuid_length));
					buffers.push_back(boost::asio::buffer(&request.version_, 1));
					buffers.push_back(boost::asio::buffer(&request.code, 1));
					buffers.push_back(boost::asio::buffer(&request.size, 4));
					
					boost::asio::write(s, buffers);

					boost::asio::read(s, boost::asio::buffer(&response, response_header_length));
					if (response.code == waiting_messages_success) {
						if (response.size == 0) {
							cout << "There are no waiting message for you :(" << std::endl;
							break;
						}

						while (s.available() > 0) {
							boost::asio::read(s, boost::asio::buffer(&msg_meta, sizeof msg_meta));
							msg_content = new char[msg_meta.size];
							boost::asio::read(s, boost::asio::buffer(msg_content, msg_meta.size));

							cout << "From: " << usernames[msg_meta.from_id] << std::endl << "Content:" << endl;
							cout.write(msg_content, msg_meta.size);
							cout << std::endl << "-------------------" << std::endl;
							delete[] msg_content;
						}
					}

					break;

				case send_message:
					if (!boost::filesystem::exists(user_file)) {
						std::cout << "Action requires registration." << std::endl;
						break;
					}

					get_clients(&s, uuid, usernames);

					std::cout << "Enter the recipient user name: ";
					std::cin.getline(username, user_max_length);
					str_rec_username = std::string(username);
					rec_uid = get_user_uid(str_rec_username, usernames);

					while (rec_uid.is_nil()) {
						std::cout << "The username you entered doesn't exists, enter again: ";
						std::cin.getline(username, user_max_length);
						std::string str_rec_username(username);
						rec_uid = get_user_uid(str_rec_username, usernames);
					}


					std::cout << "Enter the message to be sent: " << std::endl;
					std::cin.getline(message, message_max_length);


					get_clients(&s, uuid, usernames);
					std::copy(uuid.begin(), uuid.end(), request.clientId);
					request.code = send_text_message_code;;
					request.version_ = 1;

					send_message_request_meta.to_id = rec_uid;
					send_message_request_meta.Type = 1;
					send_message_request_meta.size = strlen(message);
					request.size = sizeof send_message_request_meta + send_message_request_meta.size;

					buffers.push_back(boost::asio::buffer(&request.clientId, uuid_length));
					buffers.push_back(boost::asio::buffer(&request.version_, 1));
					buffers.push_back(boost::asio::buffer(&request.code, 1));
					buffers.push_back(boost::asio::buffer(&request.size, 4));

					buffers.push_back(boost::asio::buffer(&send_message_request_meta.to_id, uuid_length));
					buffers.push_back(boost::asio::buffer(&send_message_request_meta.Type, 1));
					buffers.push_back(boost::asio::buffer(&send_message_request_meta.size, 4));

					buffers.push_back(boost::asio::buffer(&message, send_message_request_meta.size));

					boost::asio::write(s, buffers);

					boost::asio::read(s, boost::asio::buffer(&response, response_header_length));
					if (response.code == message_sent_success && response.size > 0) {

						boost::asio::read(s, boost::asio::buffer(&msg_sent_response, sizeof msg_sent_response));
						cout << "Message sent successfully to " << usernames[msg_sent_response.to_id] << "it's id is " << msg_sent_response.message_id << std::endl;
					}

					break;

				case request_symetric_key:
					if (!boost::filesystem::exists(user_file)) {
						std::cout << "Action requires registration." << std::endl;
						break;
					}
					std::cout << "From whom would you like to request a symmetric key? ";
					std::cin.getline(username, user_max_length);

					request.code = 104;
					request.size = 0;

					break;

				case send_symetric_key:
					if (!boost::filesystem::exists(user_file)) {
						std::cout << "Action requires registration." << std::endl;
						break;
					}
					std::cout << "To whom would you like to send a symmetric key? ";
					std::cin.getline(username, user_max_length);

					request.code = 104;

					break;

				default:
					std::cout << "Please enter a valid number." << std::endl;
					break;
				}
			}
			catch (std::exception& e)
			{
				std::cerr << "server responded with an error" << std::endl << e.what() << std::endl;
			}
		}

		std::cout << "Enter message: ";
		char req[user_max_length];
		std::cin.getline(req, user_max_length);
		request_length = std::strlen(req);
		boost::asio::write(s, boost::asio::buffer(req, request_length));

		char reply[message_max_length];
		size_t reply_length = boost::asio::read(s,
			boost::asio::buffer(reply, request_length));
		std::cout << "Reply is: ";
		std::cout.write(reply, reply_length);
	}
	catch (std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
	}

	return 0;
}