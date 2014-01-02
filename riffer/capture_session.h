#include <string>
#include <vector>
#include <fstream>

#include "chunk.h"

namespace rfr {
	struct FileIndexPt {
		long position;
		long value;
		FileIndexPt(long _position, long _value)
			: position(_position), value(_value) {}
	};

	struct CaptureSession {
		std::fstream * capture_file;
		std::string filename;
		CaptureSession(std::string _filename = "capture.dat", bool overwrite = true) {
			filename = _filename;
			
			std::ios_base::openmode mode = std::fstream::binary | std::fstream::in | std::fstream::out;
			if (overwrite)
				mode |= std::fstream::trunc; //dicard file contents
			else
				mode |= std::fstream::app | std::fstream::ate;
				//lol syntax

			capture_file = new std::fstream(filename, mode);
			//capture_file->open(filename, mode);
		}

		//holds chunk positions in capture_file
		std::vector<long> _chunk_index;
		//the string key in the map below is the 4-char tag itself, not the tag name.
		std::map<std::string, std::vector<FileIndexPt>> _param_index;
		void index_by(std::string tag_name) {
			//informs CaptureSession to index by the given tag.
			std::string tag = tags::get_tag(tag_name);
			if (_param_index.find(tag_name) != _param_index.end()) {
				std::cout << "We are already indexing by " << tag_name << "\n";
				return;
			}
			_param_index[tag] = std::vector<FileIndexPt>(); //add empty index.
		}

		void run_index() {
			//clears and re-does any indexing by tags it is supposed to index.
			//will take some time for larger files.
			_chunk_index.clear();

			//TODO run_index
		}

		Chunk _read_chunk_at_file_index(long file_index) {
			//should we be locking the file from other accesses here?

			capture_file->seekg(file_index);

			//TODO _read_chunk_at_file_index

			return Chunk();
		}

		Chunk get_at_index(int index) {
			return _read_chunk_at_file_index(_chunk_index[index]);
		}

		template <class T>
		Chunk get_at_index(std::string indexing_param, T indexing_value) {
			//check parameter index exists.
			std::map<std::string, std::vector<FileIndexPt>>::iterator it;
			it = _param_index.find(indexing_param);
			if (it == _param_index.end()) {
				std::cout << "We did not index by " << indexing_param << "\n";
				return Chunk();
			}

			std::vector<FileIndexPt> param_file_index = it->second;
			//do a binary search within param_file_index
			int imax = param_file_index.size();
			int imin = 0;
			int imid = -1;
			while (imax - imin > 1)
            {
				imid = (imax - imin) / 2 + imin;
				if (param_file_index[imid].value < indexing_value)
					imin = imid;
				else
					imax = imid;
			}
			//expect imin == imax
			long file_index = param_file_index[imid].position; 
			return _read_chunk_at_file_index(file_index);
		}

		void add(Chunk chunk) {
			//TODO add to capture_file

			//TODO add to index if chunk contains an indexing parameter.

		}

		void close() {
			capture_file->close();
		}

		~CaptureSession() {
			close();
		}
	};

};
