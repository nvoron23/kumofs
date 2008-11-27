#ifndef LOGIC_BASE_H__
#define LOGIC_BASE_H__

#include "kazuhiki/kazuhiki.h"
#include "log/mlogger_tty.h"
#include "log/mlogger_ostream.h"
#include "logic/global.h"
#include <mp/iothreads.h>
#include <mp/pthread.h>

namespace kumo {


class scoped_listen_tcp {
public:
	scoped_listen_tcp(struct sockaddr_in addr);
	~scoped_listen_tcp();

public:
	static int listen(const rpc::address& addr);

public:
	int sock() const
		{ return m_sock; }

	rpc::address addr() const
		{ return rpc::address(m_addr); }

private:
	rpc::address m_addr;
	int m_sock;

private:
	scoped_listen_tcp();
	scoped_listen_tcp(const scoped_listen_tcp&);
};


void do_daemonize(bool close_stdio, const char* pidfile);


struct rpc_server_args {
	rpc_server_args();
	~rpc_server_args();

	bool verbose;

	bool logfile_set;
	std::string logfile;

	bool pidfile_set;
	std::string pidfile;

	const char* prog;

	double clock_interval;  // sec
	unsigned long clock_interval_usec;  // convert

	unsigned short connect_timeout_steps;

	double reconnect_interval;  // sec
	unsigned int reconnect_timeout_msec; // convert

	unsigned short wthreads;
	unsigned short rthreads;
	unsigned short cthreads;

public:
	virtual void set_basic_args();
	virtual void show_usage();

	void parse(int argc, char** argv);

protected:
	virtual void convert();
};


struct rpc_cluster_args : rpc_server_args {
	rpc_cluster_args();
	~rpc_cluster_args();

	unsigned short connect_retry_limit;

	virtual void set_basic_args();
	virtual void show_usage();

	struct sockaddr_in cluster_addr_in;
	rpc::address cluster_addr;  // convert
	int cluster_lsock;  // convert

protected:
	virtual void convert();
};


class iothreads_server {
protected:
	template <typename Config>
	void init_iothreads(Config& cfg)
	{
		// initialize iothreads
		mp::iothreads::manager::initialize();
	
		// initialize signal handler before starting threads
		sigset_t ss;
		sigemptyset(&ss);
		sigaddset(&ss, SIGHUP);
		sigaddset(&ss, SIGINT);
		sigaddset(&ss, SIGTERM);

		s_pth.reset( new mp::pthread_signal(ss,
					get_signal_end(),
					reinterpret_cast<void*>(this)) );
	
		mp::iothreads::writer::initialize(cfg.wthreads);
		mp::iothreads::reader::initialize(cfg.rthreads);
		mp::iothreads::listener::initialize();
		mp::iothreads::connector::initialize(cfg.cthreads);
	
		// ignore SIGPIPE
		if( signal(SIGPIPE, SIG_IGN) == SIG_ERR ) {
			perror("signal");
			throw mp::system_error(errno, "signal");
		}
	}

	virtual void end_preprocess() { }

public:
	virtual void run()
	{
		mp::iothreads::run();
	}

	virtual void join()
	{
		mp::iothreads::join();
	}

public:
	void signal_end(int signo)
	{
		mp::iothreads::end();
		mp::iothreads::submit(finished);  // submit dummy function
		end_preprocess();
		LOG_INFO("end");
	}

private:
	static void finished() { }

	// avoid compile error
	typedef void (*sigend_callback)(void*, int);
	static sigend_callback get_signal_end()
	{
		sigend_callback f = &mp::object_callback<void (int)>::
			mem_fun<iothreads_server, &iothreads_server::signal_end>;
		return f;
	}

	std::auto_ptr<mp::pthread_signal> s_pth;
};


}  // namespace kumo

#endif /* logic/base.h */
