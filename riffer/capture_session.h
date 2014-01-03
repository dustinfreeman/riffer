#include <string>
#include <vector>
#include <fstream>

#include "chunk.h"

namespace rfr {
	struct FileIndexPt {
		long position;
		long value;
		//currently, only supporting indexing by long.
		//int type_id;
		FileIndexPt(long _position, long _value) //, int _type_id)
			: position(_position), value(_value) //, type_id(_type_id) 
		{}
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

			//iterate through each top-level chunk, 
			// only reading sub-chunks if they are part of the index.

			//TODO run_index
		}

		void add(Chunk chunk) {
			//go to end of capture file.
			capture_file->seekg(0, std::ios_base::end);
			long chunk_position = capture_file->tellg();

			//write chunk to file
			//top-level tag:
			capture_file->write(chunk.tag.c_str(), TAG_SIZE);
			//placeholder for eventual chunk size.
			capture_file->write("0000", RIFF_SIZE);
			//now, each sub-chunk
			std::map<std::string, std::shared_ptr<AbstractParam>>::iterator param_it;
			for (param_it = chunk.params.begin(); param_it != chunk.params.end(); param_it++) {
				long param_chunk_position = capture_file->tellg();

				//write tag
				capture_file->write(param_it->first.c_str(), TAG_SIZE);
				//placeholder for eventual chunk size.
				capture_file->write("0000", RIFF_SIZE);

				//write parameter data
				char* data;	unsigned int data_length = 0;
				switch (param_it->second->get_type_id()) {
					case INT_TYPE:
						data = chunk.get_parameter_by_tag_as_char_ptr<int>(param_it->first, &data_length);
						break;
					case LONG_TYPE:
						data = chunk.get_parameter_by_tag_as_char_ptr<long>(param_it->first, &data_length);
						break;
					case CHAR_TYPE:
						data = chunk.get_parameter_by_tag_as_char_ptr<char>(param_it->first, &data_length);
						break;
					case CHAR_PTR_TYPE:
						data = chunk.get_parameter_by_tag_as_char_ptr<char*>(param_it->first, &data_length);
						break;
				}
				capture_file->write(data, data_length);

				//write chunk size.
				long param_chunk_end_position = capture_file->tellg();
				capture_file->seekg(param_chunk_position + TAG_SIZE, std::ios_base::beg);
				capture_file->write(reinterpret_cast<char*>(param_chunk_end_position - (param_chunk_position + TAG_SIZE + RIFF_SIZE)), RIFF_SIZE);
				//go to end of chunk again.
				capture_file->seekg(param_chunk_end_position, std::ios_base::beg);
			}
			//write chunk size.
			long chunk_end_position = capture_file->tellg();
			capture_file->seekg(chunk_position + TAG_SIZE, std::ios_base::beg);
			capture_file->write(reinterpret_cast<char*>(chunk_end_position - (chunk_position + TAG_SIZE + RIFF_SIZE)), RIFF_SIZE);
			//go to end again.
			capture_file->seekg(0, std::ios_base::end);

			//index by indexing parameters.
			_chunk_index.push_back(chunk_position);
			std::map<std::string, std::vector<FileIndexPt>>::iterator it;
			for (it = _param_index.begin(); it != _param_index.end(); ++it) {
				const std::string indexing_tag = it->first;
				long* index_value = chunk.get_parameter_by_tag<long>(indexing_tag);
				if (index_value != nullptr) {
					_param_index[indexing_tag].push_back(FileIndexPt(chunk_position, *index_value));
				}
			}
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

		void close() {
			capture_file->close();
		}

		~CaptureSession() {
			close();
		}
	};

};
