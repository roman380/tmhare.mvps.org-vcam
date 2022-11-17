#pragma once

#include <stdint.h>

enum MessageType
{
	CAMERA_STATUS,
	KEEP_ALIVE
};

template <typename Payload>
struct Message
{
	Message(MessageType type, uint32_t senderId) :
		messageType{ type }, senderId{ senderId } {}
	MessageType messageType;
	uint32_t senderId;
	Payload payload;
};

template<>
struct Message<void>
{
	Message(MessageType type, uint32_t senderId) :
		messageType{ type }, senderId{ senderId } {}
	MessageType messageType;
	uint32_t senderId;
};

struct MessageKeepAlive : Message<void>
{
	MessageKeepAlive(uint32_t senderId) :
		Message<void>(MessageType::KEEP_ALIVE, senderId) {}
};

enum CameraStatus
{
	RUN = 0,
	STOP = 1,
	START_PROCESS = 2,
	STOP_PROCESS = 3
};

struct MessageCameraStatus : Message<CameraStatus>
{
	MessageCameraStatus(uint32_t senderId) :
		Message<CameraStatus>(MessageType::CAMERA_STATUS, senderId) {}
};
