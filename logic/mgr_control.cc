#include "logic/mgr_impl.h"

namespace kumo {


namespace control {
	using msgpack::define;
	using msgpack::type::tuple;
	using msgpack::type::raw_ref;
	typedef HashSpace::Seed HSSeed;

	struct Status : define< tuple<
			HSSeed,
			std::vector<address> > > {
		HSSeed& hsseed() { return get<0>(); }
		std::vector<address>& newcomers() { return get<1>(); }
	};

	enum command_type {
		GetStatus       = 84,
		StartReplace    = 85,
		CreateBackup    = 86,
	};
}  // namespace control


void Manager::GetStatus(rpc::responder response)
{
	// FIXME stub
	control::Status res;
	res.hsseed() = HashSpace::Seed(m_whs);
	for(newcomer_servers_t::iterator it(m_newcomer_servers.begin()), it_end(m_newcomer_servers.end());
			it != it_end; ++it) {
		shared_node n(it->lock());
		if(n) {
			res.newcomers().push_back(n->addr());
		}
	}
	response.result(res);
}

void Manager::StartReplace(rpc::responder response)
{
	// FIXME
	start_replace();
	msgpack::type::nil res;
	response.result(res);
}

void Manager::CreateBackup(rpc::responder response)
{
	// FIXME stub
	response.null();
}



class Manager::ControlConnection : public rpc::connection<ControlConnection> {
public:
	ControlConnection(int fd, Manager* mgr);
	~ControlConnection();

public:
	void dispatch_request(method_id method, msgobj param, rpc::responder& response, auto_zone& z);
	void process_response(msgobj result, msgobj error, msgid_t msgid, auto_zone& z);

private:
	Manager* m_mgr;

private:
	static void GetStatus(rpc::responder response, Manager* mgr);
	static void StartReplace(rpc::responder response, Manager* mgr);
	static void CreateBackup(rpc::responder response, Manager* mgr);

private:
	ControlConnection();
	ControlConnection(const ControlConnection&);
};


void Manager::control_checked_accepted(void* data, int fd)
{
	Manager* self = reinterpret_cast<Manager*>(data);
	if(fd < 0) {
		LOG_FATAL("accept failed: ",strerror(-fd));
		self->signal_end(SIGTERM);
		return;
	}
	mp::set_nonblock(fd);
	mp::iothreads::add<ControlConnection>(fd, self);
}

void Manager::listen_control(int lsock)
{
	mp::iothreads::listen(lsock,
			&Manager::control_checked_accepted,
			reinterpret_cast<void*>(this));
}

Manager::ControlConnection::ControlConnection(int fd, Manager* mgr) :
	rpc::connection<ControlConnection>(fd),
	m_mgr(mgr) { }

Manager::ControlConnection::~ControlConnection() { }


void Manager::ControlConnection::dispatch_request(method_id method, msgobj param, rpc::responder& response, auto_zone& z)
{
	LOG_TRACE("receive control message");
	switch((control::command_type)method) {
	case control::GetStatus:
		iothreads::submit(&ControlConnection::GetStatus, response, m_mgr);
		break;
	case control::StartReplace:
		iothreads::submit(&ControlConnection::StartReplace, response, m_mgr);
		break;
	case control::CreateBackup:
		iothreads::submit(&ControlConnection::CreateBackup, response, m_mgr);
		break;
	default:
		throw std::runtime_error("unknown method");
	}
}

void Manager::ControlConnection::process_response(msgobj result, msgobj error, msgid_t msgid, auto_zone& z)
{
	throw msgpack::type_error();
}


void Manager::ControlConnection::GetStatus(rpc::responder response, Manager* mgr)
{
	mgr->GetStatus(response);
}

void Manager::ControlConnection::StartReplace(rpc::responder response, Manager* mgr)
{
	mgr->StartReplace(response);
}

void Manager::ControlConnection::CreateBackup(rpc::responder response, Manager* mgr)
{
	mgr->CreateBackup(response);
}


}  // namespace kumo
