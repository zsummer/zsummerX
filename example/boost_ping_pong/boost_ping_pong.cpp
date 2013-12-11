// boost_ping_pong.cpp : 定义控制台应用程序的入口点。
//


#include <boost/asio.hpp>
#include <iostream>
#include <stdio.h>

#include <chrono>
using namespace std;
#define PACK_LEN 1000
int main(int argc, char* argv[])
{
	char buffSend[1024];
	char buffRecv[1024];
	cout <<"input 1:server, 0:client" << endl;
	int nServer = 0;
	cin >> nServer;
	
	boost::asio::io_service ios;
	boost::asio::ip::tcp::socket client(ios);

	unsigned long long recvCount = 0;
		std::function<void (const boost::system::error_code&,std::size_t, std::size_t, std::size_t)> onSend=
		[&](const boost::system::error_code& ec,std::size_t trans, std::size_t curSend, std::size_t totalNeedSend)
	{
		if (ec)
		{
			cout << " send error " << ec.message()<< endl;
			return;
		}
		curSend += trans;
		if (curSend != totalNeedSend)
		{
			client.async_write_some(boost::asio::buffer(buffSend+curSend, totalNeedSend - curSend), 
					std::bind(onSend, std::placeholders::_1, std::placeholders::_2, curSend, totalNeedSend));
		}
		else
		{
	//		cout << "send ok" << endl;
		}
	};
	std::function<void (const boost::system::error_code&,std::size_t, std::size_t, std::size_t)> onRecv=
		[&](const boost::system::error_code& ec,std::size_t trans, std::size_t curRecv, std::size_t totalNeedRecv)
	{
		if (ec)
		{
			cout << " recv error " << ec.message()<< endl;
			return;
		}
		curRecv += trans;
		if (totalNeedRecv == curRecv)
		{
//			cout << "recv one pack" << endl;
			recvCount++;
			client.async_read_some(boost::asio::buffer(buffRecv, 2), 
				std::bind(onRecv, std::placeholders::_1, std::placeholders::_2, 0, PACK_LEN));



			memcpy(buffSend, buffRecv, 1024);
			client.async_write_some(boost::asio::buffer(buffSend, PACK_LEN), 
					std::bind(onSend, std::placeholders::_1, std::placeholders::_2, 0, PACK_LEN));

		}
		else
		{
			client.async_read_some(boost::asio::buffer(buffRecv+curRecv, totalNeedRecv-curRecv), 
				std::bind(onRecv, std::placeholders::_1, std::placeholders::_2, curRecv, totalNeedRecv));
		}
	};

	boost::asio::deadline_timer timer(ios, boost::posix_time::seconds(5));
	unsigned long long lastCount = 0;
	std::function<void (const boost::system::error_code& )> onTimer =
		[&](const boost::system::error_code& ec)
	{
		if (ec)
		{
			cout << "timer error" << ec.message()<< endl;
			return;
		}
		cout << " recvSpeed=" << (recvCount - lastCount)/5.0 << endl;
		lastCount = recvCount;
		timer.expires_from_now(boost::posix_time::seconds(5));
		timer.async_wait(onTimer);
	};
	timer.async_wait(onTimer);


	if (nServer)
	{
			boost::asio::ip::tcp::endpoint ep(boost::asio::ip::tcp::v4(), 81);
			boost::asio::ip::tcp::acceptor accepter(ios, ep);
			boost::system::error_code ec;
			accepter.accept(client, ep, ec);
			if (ec)
			{
				cout << "accept error " << ec.message()<< endl;
				return 0;
			}
			cout << client.remote_endpoint().address() << ":" << client.remote_endpoint().port() << endl;
			client.async_read_some(boost::asio::buffer(buffRecv, 2), 
				std::bind(onRecv, std::placeholders::_1, std::placeholders::_2, 0, PACK_LEN));
	}
	else
	{
			boost::asio::ip::tcp::endpoint ep( boost::asio::ip::address_v4::from_string("127.0.0.1"), 81);
			boost::system::error_code ec;
			client.connect(ep, ec);
			if (ec)
			{
				cout << " connect error " << ec.message()<< endl;
				return 0;
			}
			else
			{
				cout <<"connect success" << endl;
			}
			client.async_read_some(boost::asio::buffer(buffRecv, 2), 
				std::bind(onRecv, std::placeholders::_1, std::placeholders::_2, 0, PACK_LEN));
			client.async_write_some(boost::asio::buffer(buffSend, PACK_LEN), 
					std::bind(onSend, std::placeholders::_1, std::placeholders::_2, 0, PACK_LEN));
	}


	do
	{
		boost::system::error_code ec;
		ios.run_one(ec);
	} while (true);
	
	return 0;
}

