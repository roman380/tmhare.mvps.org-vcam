#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <memory>

#include "logger.hpp"

namespace shared_queue
{
	struct video_queue;
	enum target_format 
	{
		nv12,
		i420,
		yuy2
	};

	struct resolution
	{
		int x;
		int y;
	};

	struct i420_scale 
	{
		enum target_format format;

		resolution src;
		resolution dst;
	};

	struct queue_info
	{
		resolution res;
		uint64_t interval;
	};

	typedef struct video_queue video_queue_t;
	typedef struct i420_scale i420_scale_t;

	enum queue_state {
		invalid,
		starting,
		ready,
		stopping,
	};

	class video_circular_queue
	{
	public:
		video_circular_queue(std::shared_ptr<content_camera::logger> logger);
		bool close();
		queue_info get_info();
		queue_state get_state();
	protected:
		std::unique_ptr<video_queue_t> m_queue;
		std::shared_ptr<content_camera::logger> m_logger;
	protected:
		video_circular_queue() = default;
		~video_circular_queue();
	};

	// reader can only open file mapping
	class video_queue_reader : public video_circular_queue
	{
	public:
		video_queue_reader(std::shared_ptr<content_camera::logger> logger);
		bool open();
		bool is_open();
		bool read(i420_scale_t* scale, uint8_t* dst, uint64_t* ts);
	};

	// writter is responsible for create file mapping
	// TODO - implement interfaces
	/*class video_queue_writter : public video_circular_queue
	{
	public:
		bool create(queue_info info);
		void write(uint8_t* data, const std::string& type, const queue_info& info);
	};*/



	/*std::shared_ptr<video_queue_t> video_queue_create(uint32_t cx, uint32_t cy,
		uint64_t interval);
	std::shared_ptr<video_queue_t> video_queue_open();
	void video_queue_close(std::shared_ptr<video_queue_t> vq);

	void video_queue_get_info(std::shared_ptr<video_queue_t> vq, uint32_t* cx, uint32_t* cy,
		uint64_t* interval);
	void video_queue_write(std::shared_ptr<video_queue_t> vq, uint8_t** data,
		uint32_t* linesize, uint64_t timestamp);
	queue_state video_queue_state(std::shared_ptr<video_queue_t> vq);
	bool video_queue_read(std::shared_ptr<video_queue_t> vq, i420_scale_t* scale, void* dst,
		uint64_t* ts);*/
}