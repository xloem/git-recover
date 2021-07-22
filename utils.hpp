#pragma once

class imgdata
{
public:
	imgdata(char const *pathname);
	imgdata(imgdata const & other, int64_t offset);
	~imgdata();

	void read(uint64_t offset, uint64_t length, void *buf);

	uint64_t size() const;

private:
	int fd;
	bool opened;
	int64_t adjust;
	uint64_t eof;
};

/*
template <typename T, template Storage = std::span>
class multivector
{
public:
	std::span<T> operator[](size_t idx)
	{
		return {_data.begin() + idx * _span, _data.begin() + (idx + 1) * _span};
	}
	size_t size() const {
		return _data.size() / _span;
	}

	Storage<T> _data;
	size_t _span;
};
*/

double similarity(void * buf1, void * buf2, size_t len);

#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

imgdata::imgdata(char const *pathname)
: fd(open(pathname, O_RDONLY)),
  opened(fd != -1),
  adjust(0),
  eof(0)
{
	if (fd == -1) {
		throw std::runtime_error(std::string("failed to open ") + pathname);
	}
}

imgdata::~imgdata()
{
	if (opened) {
		close(fd);
		opened = false;
	}
}

void imgdata::read(uint64_t offset, uint64_t length, void *buf)
{
	if (lseek(fd, offset, SEEK_SET) != offset) {
		throw std::runtime_error("failed to seek to " + std::to_string(offset));
	}
	eof = 0;
	int r = ::read(fd, buf, length);
	if (r == -1) {
		throw std::runtime_error("failed to read " + std::to_string(length));
	}
	if (r != length) {
		eof = r;
	}
}

uint64_t imgdata::size() const
{
	uint64_t output;
	int r = ioctl(fd, BLKGETSIZE64, &output);
	if (r) {
		struct stat s;
		r = fstat(fd, &s);
		if (r) {
			throw std::runtime_error("failed to size fd");
		}
		output = s.st_size;
	}
	return output;
}

bool same(void * buf1, void * buf2, size_t len)
{
	return mcmp(buf1, buf2, len) == 0;
}

uint8_t bitcount(unsigned long val)
{
	return __builtin_popcountl(val);
}

double similarity(void * buf1, void * buf2, size_t len)
{
	// this is an attempt to count differing bits
	// is there some library that does this?
	// could possibly be sped up a lot using intrinsics
	ssize_t ct = 0;
#define WORD_SIZE sizeof(unsigned long)
#define WORD_T unsigned long
	char * b1 = (char*)buf1;
	char * b2 = (char*)buf2;
	size_t w;

	for (w = 0; w + WORD_SIZE <= len; w += WORD_SIZE) {
		ct += bitcount(*(WORD_T*)(b1 + w) ^ *(WORD_T*)(b2 + w));
	}
	if (w != len) {
		WORD_T tail = *(WORD_T*)(b1 + w) ^ *(WORD_T*)(b2 + w);
		tail &= ~((~0UL) << ((len - w) * 8));
		ct += bitcount(tail);
	}
#undef POPCOUNT
#undef WORD_T
#undef WORD_SIZE

	len *= 8;

	// // adjustment so that 50% becomes 0.0
	// ct *= 2;
	// if (ct > len) { ct = len; }

	return (len - ct) / double(len);
}
