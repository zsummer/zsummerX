
// boost_ping_pong.cpp 
//


#include <boost/asio.hpp>
#include <iostream>
#include <stdio.h>
#include <chrono>
using namespace std;

int main(int argc, char* argv[])
{
	char buff[1024];
	int nServer = 0;
	cin >> nServer;

	unsigned long long recvCount = 0;
	if (nServer)
	{
			boost::asio::io_service ios;
			boost::asio::ip::tcp::endpoint ep(boost::asio::ip::tcp::v4(), 81);
			boost::asio::ip::tcp::acceptor accepter(ios, ep);
			boost::asio::ip::tcp::socket client(ios);
			accepter.accept(client);
			cout << client.remote_endpoint().address() << ":" << client.remote_endpoint().port() << endl;
			unsigned long long now = std::chrono::system_clock::now().time_since_epoch()/std::chrono::milliseconds(1);
			unsigned long long lastCount = 0;
			do
			{
				client.receive(boost::asio::buffer(buff, 2));
				client.receive(boost::asio::buffer(buff+2, 998));
				if (client.send(boost::asio::buffer(buff, 1000)) != 1000)
				{
					cout << " send error " << endl;
					break;
				}
				recvCount ++;
				if (std::chrono::system_clock::now().time_since_epoch()/std::chrono::milliseconds(1) - now > 5000)
				{
					cout << " recvSpeed=" << (recvCount - lastCount)/5.0 << endl;
					lastCount = recvCount;
					now = std::chrono::system_clock::now().time_since_epoch()/std::chrono::milliseconds(1);
				}
			} while (true);
	}
	else
	{
			boost::asio::io_service ios;
			boost::asio::ip::tcp::endpoint ep( boost::asio::ip::address_v4::from_string("127.0.0.1"), 81);
			boost::asio::ip::tcp::socket client(ios);
			boost::system::error_code ec;
			client.connect(ep, ec);
			if (ec)
			{
				cout << " connect error " << endl;
				return 0;
			}

			unsigned long long now = std::chrono::system_clock::now().time_since_epoch()/std::chrono::milliseconds(1);
			unsigned long long lastCount = 0;
			do
			{
				if (client.send(boost::asio::buffer(buff, 1000)) != 1000)
				{
					cout << " send error " << endl;
					break;
				}
				client.receive(boost::asio::buffer(buff, 2));
				client.receive(boost::asio::buffer(buff+2, 998));
				recvCount ++;
				if (std::chrono::system_clock::now().time_since_epoch()/std::chrono::milliseconds(1) - now > 5000)
				{
					cout << " recvSpeed=" << (recvCount - lastCount)/5.0 << endl;
					lastCount = recvCount;
					now = std::chrono::system_clock::now().time_since_epoch()/std::chrono::milliseconds(1);
				}
			} while (true);
	}


	return 0;
}

