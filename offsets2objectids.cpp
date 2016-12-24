#include <cryptopp/sha.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <zlib.h>

using namespace std;
using namespace CryptoPP;

int main()
{
	std::string filename, openFilename;
	uint8_t inBuf[65536], outBuf[65536];
	uint64_t offset;
	ifstream file;

	z_stream z; memset(&z, 0, sizeof(z));
	z.zalloc = Z_NULL;
	z.zfree = Z_NULL;
	z.next_in = inBuf;
	z.avail_in = file.gcount();
	z.next_out = outBuf;
	z.avail_out = sizeof(outBuf);
	inflateInit(&z);

	std::cin.unsetf(std::ios::dec);
	while (cin >> filename >> offset) {
		if (! file.is_open() || openFilename != filename) {
			file.open(filename, ios_base::binary);
			openFilename = filename;
		}
		file.seekg(offset);
		
		file.read(reinterpret_cast<char*>(inBuf), sizeof(inBuf));
		z.next_in = inBuf;
		z.avail_in = file.gcount();
		z.next_out = outBuf;
		z.avail_out = sizeof(outBuf);
		z.msg = 0;

		SHA1 hasher;
		byte digest[SHA1::DIGESTSIZE];

		std::string tag = "unk"; uint64_t size; bool read_tag = false;
		inflateReset(&z);
		int result;
		do {
			result = inflate(&z, Z_NO_FLUSH);
			if (!read_tag) {
				stringstream ss;
				ss << outBuf;
				ss >> tag >> size;
				read_tag = true;
			}
			hasher.Update(outBuf, sizeof(outBuf) - z.avail_out);
			z.avail_out = sizeof(outBuf);
			z.next_out = outBuf;
			if (z.avail_in == 0)
				z.next_in = inBuf;
			file.read(reinterpret_cast<char*>(z.next_in), sizeof(inBuf) - (z.next_in - inBuf));
			z.avail_in += file.gcount();
		} while (result == Z_OK);
		hasher.Final(digest);

		cout << filename << ' ' << offset << ' ' << tag << ' ';
		cout << setfill('0');
		if (z.msg || result != Z_STREAM_END) {
			cout << "result=" << result << ' ' << z.msg;
			for (size_t i = 0; i < sizeof(digest); ++ i)
				cout << ' ' << setw(2) << hex << (0+inBuf[i]);
			cout << dec;
		} else {
			for (size_t i = 0; i < sizeof(digest); ++ i)
				cout << setw(2) << hex << (0+digest[i]);
			cout << ' ' << dec << size;
		}
		cout << endl;
	}
	inflateEnd(&z);
}
