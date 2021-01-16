#include <iostream>
#include <filesystem>
#include <string>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>
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
		char* message = new char[message_max_length];
		boost::uuids::uuid uuid;
		string user_action;
		boost::filesystem::path user_file(USER_PATH);

		// get clients
		clients_response clients[max_clients];
		size_t num_of_clients;

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
					request.paylod = username;
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
					boost::asio::read(s, boost::asio::buffer(&response, 1 + 2 + 4 + uuid_length + 1));
					memcpy_s(&reg_res, uuid_length, &response.payload, uuid_length);

					if (response.code == registration_success) {

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

					
					std::copy(uuid.begin(), uuid.end(), request.clientId);
					request.code = client_list_code;
					request.size = 0;

					buffers.push_back(boost::asio::buffer(&request.clientId, uuid_length));
					buffers.push_back(boost::asio::buffer(&request.version_, 1));
					buffers.push_back(boost::asio::buffer(&request.code, 1));
					buffers.push_back(boost::asio::buffer(&request.size, 4));
					boost::asio::write(s, buffers);

					s.read_some(boost::asio::buffer(&response, response_header_length + client_list_max_bytes));

					if (response.code == 1001 && response.size > 0) {
						num_of_clients = response.size / (uuid_length + user_max_length);

						if (num_of_clients > max_clients) {
							std::cout << "Displaying only first " << max_clients << "users" << std::endl;
							memcpy_s(&clients, client_list_max_bytes, &response.payload, client_list_max_bytes);
						}
						else {
							memcpy_s(&clients, response.size, &response.payload, response.size);
						}

						for (size_t i = 0; i < response.size / (uuid_length + user_max_length); i++)
						{
							clients_response user = clients[i];
							std::string currUid = boost::uuids::to_string(user.client_id);

							std::cout << user.client_name << " - " << currUid << std::endl;
						}
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
					request.code = 104;
					request.size = 0;

					break;

				case send_message:
					if (!boost::filesystem::exists(user_file)) {
						std::cout << "Action requires registration." << std::endl;
						break;
					}
					std::cout << "To whom would you like to send a message? ";
					std::cin.getline(username, user_max_length);
					std::cout << "Enter the message to be sent: " << std::endl;
					std::cin.getline(message, message_max_length);

					request.code = 104;

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