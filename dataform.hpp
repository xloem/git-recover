#include "utils.hpp"
#include "estimate.hpp"

// this could be more reusable if
// the imgdata/offset information were separated
// from it, as a reference to possible data.
class dataform
{
public:
	// consider passed data for all other functions
	dataform(imgdata const & origin, int64_t offset);
	dataform(dataform const & origin, int64_t offset);
	void view(imgdata const & origin, int64_t offset);
	dataform() = default;

	// probability that this data has this form
	virtual double probability() = 0;

	// size of form, given data
	virtual estimate<uint64_t> const & size() = 0;

protected:
	virtual void view() = 0;
	void read(size_t offset, uint64_t length, void * buf);

private:
	imgdata const * origin;
	int64_t offset;
};

class packfile2 : public dataform
{
public:
	double probability() override
	{
		return head_similarity * entry_probability;
	}
	estimate<uint64_t> const & size() override
	{
		//return head_size;
	}
	
private:
	void view() override
	{
		read(0, 12, header);
		head_similarity = similarity(header, HEAD, 8);
		head_count = ntohl(*(uint32_t*)&header[8]);
		int64_t offset = 12;

		entry_probability = 1.0;
		pack2entry entry;
		for (size_t obj = 0; obj < head_count; obj ++) {
			entry.view(*this, offset);
			offset += entry.size();
			entry_probability *= entry.probability();
		}

		char checksum[20];
		read(offset, 20, checksum);
		// TODO: check checksum, maybe only if pack header is good, and/or only first read or if requested by user
		size = offset + 20;
	}
	char header[12];
	double head_similarity;
	double entry_probability;
	uint32_t head_count;
	uint32_t size;

	static char HEAD[8] = {'P','A','C','K',0,0,0,2};
};

class pack2entry : public dataform
{
public:
	double probability() override
	{
		// TODO: check compressed stream
		return header_similarity;
	}
	estimate<uint64_t> const & size() override
	{
		return compressed_size + header_size;
	}

	int type() const
	{
		return _type;
	}

	std::vector<uint8_t> compressed_data()
	{
		std::vector<uint8_t> data(compressed_size);
		read(header_size, compressed_size, data.data());
		return data;
	}
private:
	void view() override
	{
		// read n-byte length
		char bytes[16];
		read(0, sizeof(bytes), &bytes);

		// type 0 is defined as invalid
		// type 5 is for future expansion
		_type = (bytes[0] & 0x70) >> 4;
		bytes[0] &= 8f;
		compressed_size = bytes[0] & 0x0f;
		uint8_t size_shift = 4;

		headersize = 0;
		while (bytes[headersize] & 0x80) {
			++ headersize;
			compressed_size |= (bytes[headersize] & 0x7f) << size_shift;
			size_shift += 7;
		};
		++ headersize;

		// TODO not using probability yet.  idea: form distribution of known good data
		if (headersize > 0x10'00000000) {
			header_similarity *= 0x10 / double(0x1'00000000);
		}
		if (_type == 0 || _type == 5) {
			header_similarity *= 2.0/8;
		}
	}

	uint8_t _type;
	uint8_t header_size;
	uint64_t compressed_size;
	double header_similarity;

};

class indexfile1 : public dataform
{
public:
	double probability() override
	{
		return head_similarity;
	}
	estimate<uint64_t> const & size() override
	{
		return size;
	}

	estimate<size_t> const & subids()
	{
		return 1 + entries.size();
	}
	std::span<uint8_t> const subid(size_t idx)
	{
		if (idx == 0) {
			return packchecksum;
		} else {
			return entries[idx - 1].name;
		}
	}
	
private:
	void view() override
	{
		header_similarity = 1;
		size = sizeof(fanout)
		read(0, size, fanout);
		for (uint8_t w = 0; w < 256; w ++) {
			fanout[i] = ntohl(fanout[i]);
		}
		entries.resize(fanout[255]);
		read(size, entries.size() * sizeof(entries[0]), entries.data());
		size += entries.size() * sizeof(entries[0]);
		size_t ct = 0;
		for (uint8_t w = 0; w < 256; w ++) {
			// TODO: actual probability
			for (uint8_t x = 0; x < fanout[w]; x ++, ct ++) {
				// each entry in the fanout should have w as its start byte
				head_similarity *= similarity(entries[ct].name, &w, 1);
			}
			// each fanout count should be >= the last
			if (w > 0 && fanout[w] < fanout[w-1]) {
				head_similarity *= fanout[w] / double(0x100000000);
			}
		}
		read(size, 20, packchecksum);
		size += 20;
		char idxchecksum[20];
		read(size, 20, idxchecksum);
		size += 20;
		// TODO: check checksum?
	}
	double head_similarity;
	uint32_t fanout[256];
	struct entry {
		uint32_t offset;
		char name[20];
	};
	std::vector<entry> entries;
	char packchecksum[20];
	uint64_t size;
};

class indexfile2 : public dataform
{
public:
	double probability() override
	{
		return head_similarity;
	}
	estimate<uint64_t> const & size() override
	{
		//return head_size;
	}
	
private:
	void view() override
	{
		char header[8];
		size = 8;
		read(size, 8, header);
		head_similarity = similarity(header, HEAD, 8);
		read(size, sizeof(fanout), fanout);
		size += sizeof(fanout);
		for (uint8_t w = 0; w < 256; w ++) {
			fanout[i] = ntohl(fanout[i]);
		}
			
		int64_t offset = 12;
		for (size_t obj = 0; obj < head_count; obj ++) {
			// packobj form
		}
	}
	double head_similarity;
	uint32_t fanout[256];
	uint64_t size;
	std::vector<uint8_t> objnames;

	static char HEAD[8] = {'\377','t','0','c',0,0,0,2};
};

dataform::dataform(imgdata const & origin, int64_t offset)
{
	view(origin, offset);
}

dataform::dataform(dataform const & origin, int64_t offset)
{
	view(*origin.origin, offset + origin.offset);
}

void dataform::view(imgdata const & origin, int64_t offset)
{
	this->origin = &origin;
	this->offset = offset;
}

void dataform::read(size_t offset, uint64_t length, void * buf)
{
	origin->read(offset + this->offset, length, buf);
}
