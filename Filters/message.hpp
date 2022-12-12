#pragma once

#include <stdint.h>

enum message_type_enum
{
	camera_status,
	keep_alive
};

template <typename payload>
struct message
{
	message(message_type_enum type, uint32_t sender_id) :
		type{ type }, sender_id{ sender_id } {}
	message_type_enum type;
	uint32_t sender_id;
	payload payload;
};

template<>
struct message<void>
{
	message(message_type_enum type, uint32_t sender_id) :
		type{ type }, sender_id{ sender_id } {}
	message_type_enum type;
	uint32_t sender_id;
};

struct message_keep_alive : message<void>
{
	message_keep_alive(uint32_t sender_id) :
		message<void>(message_type_enum::keep_alive, sender_id) {}
};

enum camera_status_enum
{
	run = 0,
	stop = 1
};

struct message_camera_status : message<camera_status_enum>
{
	message_camera_status(uint32_t sender_id) :
		message<camera_status_enum>(message_type_enum::camera_status, sender_id) {}
};
