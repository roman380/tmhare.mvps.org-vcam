#include <windows.h>
#include "shared_memory_queue.hpp"
#include "Constants.hpp"

#define VIDEO_NAME L"STBVirtualCamVideo"

struct queue_header {
	volatile uint32_t write_idx;
	volatile uint32_t read_idx;
	volatile uint32_t state;

	uint32_t offsets[3];

	uint32_t type;

	uint32_t cx;
	uint32_t cy;
	uint64_t interval;

	uint32_t reserved[8];
};

struct shared_queue::video_queue {
	HANDLE handle;
	bool ready_to_read;
	queue_header* header;
	uint64_t* ts[3];
	uint8_t* frame[3];
	long last_inc;
	int dup_counter;
	bool is_writer;
};

#define ALIGN_SIZE(size, align) size = (((size) + (align - 1)) & (~(align - 1)))
#define FRAME_HEADER_SIZE 32

void copy_from_queue(shared_queue::i420_scale_t* s, uint8_t* dst, const uint8_t* src)
{
	memcpy(dst, src, s->src.x * s->src.y * 3 / 2);
}

#define get_idx(inc) ((unsigned long)inc % 3)

bool shared_queue::video_circular_queue::close()
{
	if (!m_queue) {
		TRY_LOG(warn("Cannot close video queue: already closed"));
		return false;
	}
	if (m_queue->is_writer) {
		m_queue->header->state = queue_state::stopping;
	}

	UnmapViewOfFile(m_queue->header);
	CloseHandle(m_queue->handle);
	m_queue = nullptr;
	TRY_LOG(info("Video queue closed"));
	return true;
}

shared_queue::queue_info shared_queue::video_circular_queue::get_info()
{
	queue_header* qh = m_queue->header;
	queue_info info;
	info.res.x = qh->cx;
	info.res.y = qh->cy;
	info.interval = qh->interval;
	return info;
}

shared_queue::queue_state shared_queue::video_circular_queue::get_state()
{
	if (!m_queue) 
		return queue_state::invalid;

	auto state = (queue_state)m_queue->header->state;
	if (!m_queue->ready_to_read && state == queue_state::ready)
	{
		for (size_t i = 0; i < 3; i++) {
			size_t off = m_queue->header->offsets[i];
			m_queue->ts[i] = (uint64_t*)(((uint8_t*)m_queue->header) + off);
			m_queue->frame[i] = ((uint8_t*)m_queue->header) + off +
				FRAME_HEADER_SIZE;
		}
		m_queue->ready_to_read = true;
	}

	return state;
}

shared_queue::video_circular_queue::~video_circular_queue()
{
	close();
}

bool shared_queue::video_queue_reader::open()
{
	auto vq = std::make_shared<video_queue>();

	vq->handle = OpenFileMappingW(FILE_MAP_READ, false, VIDEO_NAME);
	if (!vq->handle) {
		return false;
	}

	vq->header = (queue_header*)MapViewOfFile(
		vq->handle, FILE_MAP_READ, 0, 0, 0);
	if (!vq->header) {
		TRY_LOG(error("Cannot open video queue: MapVidevOfFile failed"));
		CloseHandle(vq->handle);
		return false;
	}
	m_queue = vq;
	TRY_LOG(info("Video queue open"));
	return true;
}

bool shared_queue::video_queue_reader::is_open()
{
	return m_queue != nullptr;
}

bool shared_queue::video_queue_reader::read(i420_scale_t* scale, uint8_t* dst, uint64_t* ts)
{
	queue_header* qh = m_queue->header;
	long inc = qh->read_idx;

	if (qh->state == queue_state::stopping || qh->state == queue_state::invalid) {
		TRY_LOG(warn("Cannot read frame from video queue: wrong state"));
		return false;
	}

	if (inc == m_queue->last_inc) {
		if (++m_queue->dup_counter == 100) {
			TRY_LOG(warn("Cannot read frame from video queue: duplicate frames"));
			return false;
		}
	}
	else {
		m_queue->dup_counter = 0;
		m_queue->last_inc = inc;
	}

	unsigned long idx = get_idx(inc);

	*ts = *m_queue->ts[idx];

	copy_from_queue(scale, dst, m_queue->frame[idx]);
	return true;
}
