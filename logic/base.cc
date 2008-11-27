#include "logic/base.h"
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <mp/utility.h>

namespace kumo {


scoped_listen_tcp::scoped_listen_tcp(struct sockaddr_in addr) :
	m_addr(addr),
	m_sock(listen(m_addr)) { }

scoped_listen_tcp::~scoped_listen_tcp()
{
	::close(m_sock);
}


int scoped_listen_tcp::listen(const rpc::address& addr)
{
	int lsock = socket(PF_INET, SOCK_STREAM, 0);
	if(lsock < 0) {
		throw std::runtime_error("socket failed");
	}

	int on = 1;
	if( ::setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0 ) {
		::close(lsock);
		throw std::runtime_error("setsockopt failed");
	}

	char addrbuf[addr.addrlen()];
	addr.getaddr((sockaddr*)addrbuf);

	if( ::bind(lsock, (sockaddr*)addrbuf, sizeof(addrbuf)) < 0 ) {
		::close(lsock);
		throw std::runtime_error("bind failed");
	}

	if( ::listen(lsock, 8) < 0 ) {
		::close(lsock);
		throw std::runtime_error("listen failed");
	}

	mp::set_nonblock(lsock);

	return lsock;
}


void do_daemonize(bool close_stdio, const char* pidfile)
{
	pid_t pid;
	pid = fork();
	if(pid < 0) { perror("fork"); exit(1); }
	if(pid != 0) { exit(0); }
	if(setsid() == -1) { perror("setsid"); exit(1); }
	pid = fork();
	if(pid < 0) { perror("fork"); exit(1); }
	if(pid != 0) { exit(0); }
	if(pidfile) {
		FILE* f = fopen(pidfile, "w");
		if(!f) { perror("can't open pid file"); exit(1); }
		fprintf(f, "%d", getpid());
		fclose(f);
	}
	if(close_stdio) {
		int devnull_r = open("/dev/null", O_RDONLY);
		if(devnull_r < 0) { perror("open(\"/dev/null\", \"r\")"); exit(1); }
		int devnull_a = open("/dev/null", O_APPEND);
		if(devnull_a < 0) { perror("open(\"/dev/null\"), \"a\""); exit(1); }
		close(0);
		close(1);
		close(2);
		if(dup2(devnull_r, 0) < 0) { perror("dup2"); exit(1); }
		if(dup2(devnull_a, 1) < 0) { perror("dup2"); exit(1); }
		if(dup2(devnull_a, 2) < 0) { perror("dup2"); exit(1); }
		close(devnull_r);
		close(devnull_a);
	}
}


rpc_server_args::rpc_server_args() :
	clock_interval(0.5),
	connect_timeout_steps(3),
	reconnect_interval(1.0),
	wthreads(2),
	rthreads(4),
	cthreads(2)
{
	kazuhiki::init();
}

rpc_server_args::~rpc_server_args() { }

rpc_cluster_args::rpc_cluster_args() :
	connect_retry_limit(4),
	cluster_lsock(-1) { }

rpc_cluster_args::~rpc_cluster_args()
{
	::close(cluster_lsock);
}


void rpc_server_args::set_basic_args()
{
	using namespace kazuhiki;
	on("-v", "--verbose",
			type::boolean(&verbose));
	on("-o", "--log", &logfile_set,
			type::string(&logfile));
	on("-d", "--daemon", &pidfile_set,
			type::string(&pidfile));
	on("-IC", "--clock-interval",
			type::numeric(&clock_interval, clock_interval));
	on("-SY", "--connect-timeout-steps",
			type::numeric(&connect_timeout_steps, connect_timeout_steps));
	on("-IR", "--reconnect-interval",
			type::numeric(&reconnect_interval, reconnect_interval));
	on("-TW", "--write-threads",
			type::numeric(&wthreads, wthreads));
	on("-TR", "--read-threads",
			type::numeric(&rthreads, rthreads));
	on("-TC", "--connect-threads",
			type::numeric(&cthreads, cthreads));
}

void rpc_cluster_args::set_basic_args()
{
	using namespace kazuhiki;
	on("-l",  "--listen",
			type::connectable(&cluster_addr_in, CLUSTER_DEFAULT_PORT));
	on("-NY", "--connect-retry-limit",
			type::numeric(&connect_retry_limit, connect_retry_limit));
	rpc_server_args::set_basic_args();
}

void rpc_server_args::show_usage()
{
std::cout <<
"  -v                   --verbose\n"
"  -o  <path.log>       --log\n"
"  -d  <path.pid>       --daemon\n"
"  -IC <number="<<clock_interval<<">    --clock-interval      clock interval in seconds\n"
"  -SY <number="<<connect_timeout_steps<<">    --connect-timeout-steps      connect timeout sptes\n"
"  -IR <number="<<reconnect_interval<<">    --reconnect-interval  reconnect inverval in seconds\n"
"  -TW <number="<<wthreads<<">    --write-threads        number of threads for asynchronous writing\n"
"  -TR <number="<<rthreads<<">    --read-threads         number of threads for asynchronous reading\n"
"  -TC <number="<<cthreads<<">    --connect-threads      number of threads for asynchronous connecting\n"
<< std::endl;
}

void rpc_cluster_args::show_usage()
{
std::cout <<
"  -l  <addr[:port="<<CLUSTER_DEFAULT_PORT<<"]>   --listen      listen address\n"
"  -c  <[addr:]port="<<CLIENT_DEFAULT_PORT<<">    --client        client listen port\n"
"  -NY <number="<<connect_retry_limit<<">           --connect-retry-limit      connect retry limit\n"
;
rpc_server_args::show_usage();
}

void rpc_server_args::parse(int argc, char** argv)
try {
	prog = argv[0];
	--argc;
	++argv;
	kazuhiki::break_order(argc, argv);

	convert();

} catch (std::runtime_error& e) {
	show_usage();
	std::cerr << "error: " << e.what() << std::endl;
	exit(1);
}

void rpc_server_args::convert()
{
	clock_interval_usec = clock_interval * 1000 * 1000;
	reconnect_timeout_msec = reconnect_interval * 1000;
}

void rpc_cluster_args::convert()
{
	cluster_addr = rpc::address(cluster_addr_in);
	cluster_lsock = scoped_listen_tcp::listen(cluster_addr);
	rpc_server_args::convert();
}


}  // namespace kumo
