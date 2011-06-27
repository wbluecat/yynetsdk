#include "stdafx.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <iostream>

using namespace std;
using boost::asio::ip::tcp;

class session
{
public:
	session(boost::asio::io_service& ios): m_socket(ios)
	{

	}

	tcp::socket& socket()
	{
		return m_socket;
	}

	void start()
	{
		boost::asio::async_read_until(m_socket, m_recvbuf, "\n",
			boost::bind(&session::handle_read, this,
			boost::asio::placeholders::error));

		//m_socket.async_read_some(boost::asio::buffer(data_, max_length),
		//	boost::bind(&session::handle_read, this,
		//	boost::asio::placeholders::error,
		//	boost::asio::placeholders::bytes_transferred));

		//boost::asio::async_read()
	}

	void handle_read(const boost::system::error_code& err)
	{
		if(!err)
		{
			string msg;
			std::istream recv_stream(&m_recvbuf);
			std::getline(recv_stream, msg);

			std::ostream send_stream(&m_sendbuf);
			send_stream << msg;

			cout << "get: " << msg << endl;

			boost::asio::async_write(m_socket,
				m_sendbuf,
				boost::bind(&session::handle_write, this,
				boost::asio::placeholders::error));

			//boost::asio::async_write(m_socket,
			//	boost::asio::buffer(data_, bytes_transferred),
			//	boost::bind(&session::handle_write, this,
			//	boost::asio::placeholders::error));
		}
		else
		{
			cout << "read end, close session" << endl;
			delete this;
		}
	}

	void handle_write(const boost::system::error_code& err)
	{
		if(!err)
		{
			boost::asio::async_read_until(m_socket, m_recvbuf, "\n",
				boost::bind(&session::handle_read, this,
				boost::asio::placeholders::error));
		}
		else
		{
			delete this;
		}
	}

private:
	tcp::socket m_socket;
	boost::asio::streambuf m_recvbuf;
	boost::asio::streambuf m_sendbuf;

	////+
	//enum { max_length = 1024 };
	//char data_[max_length];
};

//typedef boost::shared_ptr<session> session_ptr;
typedef session* session_ptr;

class server
{
public:
	server(boost::asio::io_service& ios, short port):
	  m_ios(ios), m_acceptor(m_ios, tcp::endpoint(tcp::v4(), port))
	  {
		  cout << "start listen on port: " << port << endl;
		  do_accept();
	  }

	  void do_accept()
	  {
		  //session *new_session = new session(m_ios);
		  session_ptr new_session(new session(m_ios));
		  m_acceptor.async_accept(new_session->socket(),
			  boost::bind(&server::handle_accept, this, new_session,
			  boost::asio::placeholders::error));
	  }

	  void handle_accept(session_ptr new_session,
		  const boost::system::error_code& error)
	  {
		  if(!error)
		  {
			  cout << "accept connection ..." << endl;
			  new_session->start();
			  do_accept();
		  }
		  else
		  {
			  delete new_session;
		  }
	  }

private:
	boost::asio::io_service& m_ios;
	tcp::acceptor m_acceptor;
};

int _tmain(int argc, _TCHAR* argv[])
{
	boost::asio::io_service ios;
	server line_svr(ios, 6002);
	ios.run();

	return 0;
}

