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

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

imgdata::imgdata(char const *pathname)
: fd(open(pathname, O_RDONLY)),
  opened(fd != -1),
  adjust(0),
  eof(0)
{
	if (fd == -1) {
		throw std::exception(std::string("failed to open ") + pathname);
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
		throw std::exception("failed to seek to " + std::to_string(offset));
	}
	eof = 0;
	int r = read(fd, buf, length);
	if (r == -1) {
		throw std::exception("failed to read " + std::to_string(length));
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
		statbuf s;
		r = fstat(fd, &s);
		if (r) {
			throw std::exception("failed to size fd");
		}
		output = s.st_size;
	}
	return output;
}
