// boost_ping_pong.cpp : 定义控制台应用程序的入口点。
//


#include <boost/asio.hpp>
#include <iostream>
#include <stdio.h>
#include "../../depends/protocol4z/protocol4z.h"
#include <chrono>
using namespace std;
#define PACK_LEN 1024
int main(int argc, char* argv[])
{
	char buffSend[PACK_LEN];
	char buffRecv[PACK_LEN];
	std::string fillstring;
	fillstring.resize(1000,'z');
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
	auto sendOnce=[&](void)
	{
		zsummer::protocol4z::WriteStream ws(buffSend, PACK_LEN);
		ws << (unsigned short) 1; //protocol id
		ws <<(unsigned long long) 1; // local tick count
		ws << fillstring; // append text, fill the length protocol.
		client.async_write_some(boost::asio::buffer(buffSend, ws.GetWriteLen()), 
					std::bind(onSend, std::placeholders::_1, std::placeholders::_2, 0, ws.GetWriteLen()));
	};
	std::function<void (const boost::system::error_code&,std::size_t, std::size_t)> onRecv=
		[&](const boost::system::error_code& ec,std::size_t trans, std::size_t curRecv)
	{
		if (ec)
		{
			cout << " recv error " << ec.message()<< endl;
			return;
		}
		curRecv += trans;
		int needRecv = zsummer::protocol4z::CheckBuffIntegrity(buffRecv, curRecv, PACK_LEN);
		if ( needRecv == -1)
		{
			cout <<"killed socket: CheckBuffIntegrity error " <<endl;
			return ;
		}
		if (needRecv > 0)
		{
			client.async_read_some(boost::asio::buffer(buffRecv+curRecv, needRecv), 
				std::bind(onRecv, std::placeholders::_1, std::placeholders::_2, curRecv));
			return ;
		}

		//! 解包完成 进行消息处理
		zsummer::protocol4z::ReadStream rs(buffRecv, curRecv);
		try
		{
			//协议流异常会被上层捕获并关闭连接
			unsigned short protocolID = 0;
			rs >> protocolID;
			switch (protocolID)
			{
			case 1:
				{
					unsigned long long localTick = 0;
					std::string text;
					rs >> localTick >> text;
				}
				break;
			default:
				{
					cout <<"unknown protocol id=" << protocolID <<endl;
				}
				break;
			}
		}
		catch (std::runtime_error e)
		{
			cout << "MessageEntry catch one exception: "<< e.what()<<endl;
			return ;
		}
		recvCount++;
		client.async_read_some(boost::asio::buffer(buffRecv, 2), 
			std::bind(onRecv, std::placeholders::_1, std::placeholders::_2, 0));


		sendOnce();

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
				std::bind(onRecv, std::placeholders::_1, std::placeholders::_2, 0));
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
				std::bind(onRecv, std::placeholders::_1, std::placeholders::_2, 0));
			sendOnce();
			

	}


	do
	{
		boost::system::error_code ec;
		ios.run_one(ec);
	} while (true);
	
	return 0;
}

