#include "storage.h"
#include <queue>
#include <stdlib.h>
#include <string.h>
#include <mp/source.h>

namespace kumo {


class buffer_queue {
public:
	buffer_queue();
	~buffer_queue();

public:
	void push(const void* buf, size_t buflen);
	const void* front(size_t* result_buflen) const;
	void pop();

	size_t total_size() const;

private:
	size_t m_total_size;

	struct entry {
		void* data;
		size_t size;
	};

	typedef std::queue<entry> queue_type;
	queue_type m_queue;

	mp::source<128, 2048> m_source;
};

buffer_queue::buffer_queue() :
	m_total_size(0) { }

buffer_queue::~buffer_queue()
{
	// source::~source frees all memory
	//for(queue_type::iterator it(m_queue.begin()),
	//		it_end(m_queue.end()); it != it_end; ++it) {
	//	m_source.free(*it);
	//}
}

void buffer_queue::push(const void* buf, size_t buflen)
{
	void* data = m_source.malloc(buflen);
	::memcpy(data, buf, buflen);

	entry e = {data, buflen};
	try {
		m_queue.push(e);
	} catch (...) {
		m_source.free(data);
		throw;
	}

	m_total_size += buflen;
}

const void* buffer_queue::front(size_t* result_buflen) const
{
	if(m_queue.empty()) {
		return NULL;
	}

	const entry& e = m_queue.front();

	*result_buflen = e.size;
	return e.data;
}

void buffer_queue::pop()
{
	entry& e = m_queue.front();

	m_total_size -= e.size;
	m_source.free( e.data );

	m_queue.pop();
}

size_t buffer_queue::total_size() const
{
	return m_total_size;
}


}  // namespace kumo


using kumo::buffer_queue;

kumo_buffer_queue* kumo_buffer_queue_new(void)
try {
	buffer_queue* impl = new buffer_queue();
	return reinterpret_cast<kumo_buffer_queue*>(impl);
} catch (...) {
	return NULL;
}

void kumo_buffer_queue_free(kumo_buffer_queue* bq)
try {
	buffer_queue* impl = reinterpret_cast<buffer_queue*>(bq);
	delete impl;
} catch (...) { }

bool kumo_buffer_queue_push(kumo_buffer_queue* bq, const void* buf, size_t buflen)
try {
	buffer_queue* impl = reinterpret_cast<buffer_queue*>(bq);
	impl->push(buf, buflen);
	return true;
} catch (...) {
	return false;
}

const void* kumo_buffer_queue_front(kumo_buffer_queue* bq, size_t* result_buflen)
{
	buffer_queue* impl = reinterpret_cast<buffer_queue*>(bq);
	return impl->front(result_buflen);
}

void kumo_buffer_queue_pop(kumo_buffer_queue* bq)
try {
	buffer_queue* impl = reinterpret_cast<buffer_queue*>(bq);
	impl->pop();
} catch (...) { }

size_t kumo_buffer_queue_total_size(kumo_buffer_queue* bq)
{
	buffer_queue* impl = reinterpret_cast<buffer_queue*>(bq);
	return impl->total_size();
}


