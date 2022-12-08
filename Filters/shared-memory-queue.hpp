#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <memory>

struct video_queue;
enum target_format {
	TARGET_FORMAT_NV12,
	TARGET_FORMAT_I420,
	TARGET_FORMAT_YUY2,
};

struct nv12_scale {
	enum target_format format;

	int src_cx;
	int src_cy;

	int dst_cx;
	int dst_cy;
};

typedef struct video_queue video_queue_t;
typedef struct nv12_scale nv12_scale_t;

enum queue_state {
	SHARED_QUEUE_STATE_INVALID,
	SHARED_QUEUE_STATE_STARTING,
	SHARED_QUEUE_STATE_READY,
	SHARED_QUEUE_STATE_STOPPING,
};

extern std::shared_ptr<video_queue_t> video_queue_create(uint32_t cx, uint32_t cy,
	uint64_t interval);
extern std::shared_ptr<video_queue_t> video_queue_open();
extern void video_queue_close(std::shared_ptr<video_queue_t> vq);

extern void video_queue_get_info(std::shared_ptr<video_queue_t> vq, uint32_t* cx, uint32_t* cy,
	uint64_t* interval);
extern void video_queue_write(std::shared_ptr<video_queue_t> vq, uint8_t** data,
	uint32_t* linesize, uint64_t timestamp);
extern enum queue_state video_queue_state(std::shared_ptr<video_queue_t> vq);
extern bool video_queue_read(std::shared_ptr<video_queue_t> vq, nv12_scale_t* scale, void* dst,
	uint64_t* ts);
