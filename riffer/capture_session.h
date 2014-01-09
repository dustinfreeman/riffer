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

	const int BUFFER_SIZE = 1024;
	union data_buffer 
	{
		char	ch_ptr[BUFFER_SIZE];
		int		i;
		long	l;
		float	f;
	};

	struct CaptureSession {
		std::fstream* capture_file;
		std::string filename;
		CaptureSession(std::string _filename = "./capture.dat", bool overwrite = true) {
			filename = _filename;
			
			std::ios_base::openmode mode = std::fstream::binary | std::fstream::in | std::fstream::out;
			//mode |= std::fstream::ate; //create
			if (overwrite)
				mode |= std::fstream::trunc; //dicard file contents
			else
				mode |= std::fstream::app | std::fstream::ate;
				//lol syntax

			//capture_file->open(filename, mode);
			capture_file = new std::fstream(filename, mode);
			if (!capture_file->is_open()) {
				std::cout << "Could not open capture file.\n";
			}
		}

		//holds chunk positions in capture_file
		std::vector<long> _chunk_index;
		//the string key in the map below is the 4-char tag itself, not the tag name.
		std::map<std::string, std::vector<FileIndexPt>> _param_index;
		std::map<std::string, std::vector<FileIndexPt>>::iterator _param_index_it;
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

			//clear current index
			_chunk_index.clear();
			for (_param_index_it = _param_index.begin(); 
				_param_index_it != _param_index.end();
				_param_index_it++) {
				_param_index_it->second.clear();
			}

			capture_file->seekg(0, std::ios_base::beg);
			data_buffer buffer;

			//iterate through each top-level chunk, 
			// only reading sub-chunks if they are part of the index.
			while(!capture_file->eof()) {
				//get chunk start
				long chunk_position = capture_file->tellg(); 
				_chunk_index.push_back(chunk_position);

				//chunk tag
				capture_file->read(buffer.ch_ptr, TAG_SIZE);
				std::string tag = std::string(buffer.ch_ptr, TAG_SIZE);
				//chunk length
				capture_file->read(buffer.ch_ptr, RIFF_SIZE);
				int chunk_length = buffer.i;

				//look at each sub-chunk
				if (_param_index.size() == 0)
					continue; //no indexing of sub-params.
				while ((long)capture_file->tellg() - chunk_position < chunk_length + TAG_SIZE + RIFF_SIZE) {
					long sub_chunk_position = capture_file->tellg();
					
					//sub-chunk tag
					capture_file->read(buffer.ch_ptr, TAG_SIZE);
					std::string sub_tag = std::string(buffer.ch_ptr, TAG_SIZE);
					//sub-chunk length
					capture_file->read(buffer.ch_ptr, RIFF_SIZE);
					int sub_chunk_length = buffer.i;

					//are we indexing by this tag?
					_param_index_it = _param_index.find(sub_tag);
					if (_param_index_it != _param_index.end()) {
						//get value - only INT and LONG supported.
						long value;
						switch(tags::get_type_id_from_tag(sub_tag)) {
							case INT_TYPE:
								capture_file->read(buffer.ch_ptr, sizeof(int));
								value = buffer.i;
								break;
							case LONG_TYPE:
								capture_file->read(buffer.ch_ptr, sizeof(long));
								value = buffer.l;
								break;
						}
						_param_index[sub_tag].push_back(FileIndexPt(chunk_position, value));
					}
					//advance to end of sub-chunk.
					capture_file->seekg(sub_chunk_position + TAG_SIZE + RIFF_SIZE + sub_chunk_length, std::ios_base::beg);
				}
			}
		}

		void add(Chunk chunk) {
			//go to end of capture file.
			if (!capture_file->is_open()) {
				std::cout << "Capture file not open.\n";
			}
			capture_file->seekg(0, std::ios_base::end);
			long chunk_position; 
			chunk_position = capture_file->tellp();
			std::cout << "adding: chunk_position tell p " << chunk_position << "\n";

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
					case CHAR_PTR_TYPE:
						data = chunk.get_parameter_by_tag_as_char_ptr<char*>(param_it->first, &data_length);
						break;
				}
				capture_file->write(data, data_length);

				//write chunk size.
				long param_chunk_end_position = capture_file->tellg();
				capture_file->seekg(param_chunk_position + TAG_SIZE, std::ios_base::beg);
				int chunk_size = param_chunk_end_position - (param_chunk_position + TAG_SIZE + RIFF_SIZE);
				capture_file->write(reinterpret_cast<char*>(chunk_size), RIFF_SIZE);
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
			Chunk chunk = Chunk();

			data_buffer buffer;
			//char* buffer = new char; //2^16 bytes -- overkill?

			//chunk tag
			capture_file->read(buffer.ch_ptr, TAG_SIZE);
			chunk.tag = std::string(buffer.ch_ptr, TAG_SIZE);
			//chunk length
			capture_file->read(buffer.ch_ptr, RIFF_SIZE);
			int chunk_length = buffer.i;

			//read sub-chunks while still inside the chunk.
			while ((long)capture_file->tellg() - file_index < chunk_length + TAG_SIZE + RIFF_SIZE) {
				//sub-chunk tag
				capture_file->read(buffer.ch_ptr, TAG_SIZE);
				std::string sub_tag = std::string(buffer.ch_ptr, TAG_SIZE);
				//sub-chunk length
				capture_file->read(buffer.ch_ptr, RIFF_SIZE);
				int sub_chunk_length = buffer.i;
				//sub-chunk data
				switch(tags::get_type_id_from_tag(sub_tag)) {
					case INT_TYPE:
						if (sub_chunk_length != sizeof(int))
							std::cout << "sub_chunk_length for int unexpected size" << sub_chunk_length << "\n";
						capture_file->read(buffer.ch_ptr, sizeof(int));
						chunk.add_parameter_by_tag<int>(sub_tag, buffer.i);
						break;
					case LONG_TYPE:
						if (sub_chunk_length != sizeof(long))
							std::cout << "sub_chunk_length for long unexpected size" << sub_chunk_length << "\n";
						capture_file->read(buffer.ch_ptr, sizeof(long));
						chunk.add_parameter_by_tag<long>(sub_tag, buffer.i);
						break;
					case CHAR_PTR_TYPE: 
						{
							char* buffer_ptr = new char[sub_chunk_length];
							capture_file->read(buffer_ptr, sub_chunk_length);
							chunk.add_parameter_by_tag<char*>(sub_tag, buffer_ptr);
						}
						break;
					case UNDEFN_TYPE:
						std::cout << "Found tag of undefined type: " << sub_tag << "\n";
						break;
				}
			}
			//capture_file is now at end of chunk.
			
			return chunk;
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
			std::cout << "closing file." << (void*)capture_file << "\n";
			capture_file->close();
		}

		~CaptureSession() {
			close();
		}
	};

};
