#pragma once

#include <stdint.h>

enum MessageType
{
	CAMERA_STATUS
};

template <typename Payload>
struct Message
{
	Message(MessageType type) : messageType{ type } {}
	MessageType messageType;
	Payload payload;
};

enum CameraStatus
{
	RUN = 0,
	STOP = 1
};

struct MessageCameraStatus : Message<CameraStatus>
{
	MessageCameraStatus() :Message<CameraStatus>(MessageType::CAMERA_STATUS) {}
};
