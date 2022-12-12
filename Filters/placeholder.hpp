#pragma once

#include <string>
#include <vector>

class placeholder
{
public:
	inline uint8_t* get_placeholder_ptr() { return !data.empty() ? data.data() : nullptr; }
	void initialize_placeholder();
	void load_placeholder();
private:
	std::vector<uint8_t> data;
	int cx, cy;
};
