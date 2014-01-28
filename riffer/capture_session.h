#include <string>
#include <algorithm>    // std::find
#include <vector>
#include <fstream>

#ifndef RFR_CS
#define RFR_CS

#include "chunk.h"

namespace rfr {
	template <class T>
	struct FileIndexPt {
		std::streamoff position;
		T value;
		std::string chunk_tag;
		//int type_id;
		FileIndexPt(std::streamoff _position, T _value, std::string _chunk_tag = "") //, int _type_id)
			: position(_position), value(_value), chunk_tag(_chunk_tag) //, type_id(_type_id) 
		{ }
	};

	const int BUFFER_SIZE = 1024;
	union data_buffer 
	{
		char	ch_ptr[BUFFER_SIZE];
		int		i;
		int64_t	i64;
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

		//holds chunk positions and chunk tags in capture file.
		std::vector<FileIndexPt<std::string>> _chunk_index;
		//the string key in the map below is the 4-char tag itself, not the tag name.
		std::map<std::string, std::vector<FileIndexPt<int64_t>>> _param_index;
		std::map<std::string, std::vector<FileIndexPt<int64_t>>>::iterator _param_index_it;
		void index_by(std::string tag_name) {
			//informs CaptureSession to index by the given tag.
			std::string tag = tags::get_tag(tag_name);
			if (_param_index.find(tag_name) != _param_index.end()) {
				std::cout << "We are already indexing by " << tag_name << "\n";
				return;
			}
			
			_param_index[tag] = std::vector<FileIndexPt<int64_t>>(); //add empty index.
		}

		void run_index() {
			//clears and re-does any indexing by tags it is supposed to index.
			//will take some time for larger files.
			if (!capture_file->is_open()) {
				std::cout << "Cannot index. Capture file is not open!\n";
				return;
			}

			capture_file->seekg(0, std::ios_base::end);
			std::streamoff file_end = capture_file->tellg();

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
				std::streamoff chunk_position = capture_file->tellg(); 
				if(chunk_position >= file_end)
					break; //apparently eof doesn't work well enough?

				//chunk tag
				capture_file->read(buffer.ch_ptr, TAG_SIZE);
				std::string tag = std::string(buffer.ch_ptr, TAG_SIZE);
				//chunk length
				capture_file->read(buffer.ch_ptr, RIFF_SIZE);
				int chunk_length = buffer.i;
				
				//add to index.
				_chunk_index.push_back(FileIndexPt<std::string>(chunk_position, tag));

				//look at each sub-chunk
				if (_param_index.size() == 0)
					continue; //no indexing of sub-params.
				while ((std::streamoff)capture_file->tellg() - chunk_position < chunk_length + TAG_SIZE + RIFF_SIZE) {
					std::streamoff sub_chunk_position = capture_file->tellg();
					
					//sub-chunk tag
					capture_file->read(buffer.ch_ptr, TAG_SIZE);
					std::string sub_tag = std::string(buffer.ch_ptr, TAG_SIZE);
					//sub-chunk length
					capture_file->read(buffer.ch_ptr, RIFF_SIZE);
					int sub_chunk_length = buffer.i;

					//are we indexing by this tag?
					_param_index_it = _param_index.find(sub_tag);
					if (_param_index_it != _param_index.end()) {
						//get value - only INT and int64_t supported.
						int64_t value;
						switch(tags::get_type_id_from_tag(sub_tag)) {
							case INT_TYPE:
								capture_file->read(buffer.ch_ptr, sizeof(int));
								value = buffer.i;
								break;
							case INT_64_TYPE:
								capture_file->read(buffer.ch_ptr, sizeof(int64_t));
								value = buffer.i64;
								break;
						}
						_param_index[sub_tag].push_back(FileIndexPt<int64_t>(chunk_position, value));
					}
					//advance to end of sub-chunk.
					capture_file->seekg(sub_chunk_position + TAG_SIZE + RIFF_SIZE + sub_chunk_length, std::ios_base::beg);
				}
			}
		}

		void _add_param(Chunk chunk, std::string param_tag) {
			//helper function for add() below.
			//writes the param of the chunk to the file.
			std::streamoff param_chunk_position = capture_file->tellg();

			//write tag
			capture_file->write(param_tag.c_str(), TAG_SIZE);
			//placeholder for eventual chunk size.
			capture_file->write("0000", RIFF_SIZE);

			//write parameter data
			const char* data;	unsigned int data_length = 0;
			switch (chunk.params[param_tag]->get_type_id()) {
				case INT_TYPE:
					data = chunk.get_parameter_by_tag_as_char_ptr<int>(param_tag, &data_length);
					break;
				case INT_64_TYPE:
					data = chunk.get_parameter_by_tag_as_char_ptr<int64_t>(param_tag, &data_length);
					break;
				case CHAR_PTR_TYPE:
					data = chunk.get_parameter_by_tag_as_char_ptr<char*>(param_tag, &data_length);
					break;
				default:
					std::cout << "Unknown data type!\n";
					break;
			}
			capture_file->write(data, data_length);

			//write chunk size.
			std::streamoff param_chunk_end_position = capture_file->tellg();
			capture_file->seekg(param_chunk_position + TAG_SIZE, std::ios_base::beg);
			int param_chunk_size = param_chunk_end_position - (param_chunk_position + TAG_SIZE + RIFF_SIZE);
			capture_file->write(reinterpret_cast<const char*>(&param_chunk_size), RIFF_SIZE);
			//go to end of chunk again.
			capture_file->seekg(param_chunk_end_position, std::ios_base::beg);
		}

		void add(Chunk chunk) {
			//adds and writes the chunk data to capture_file

			//go to end of capture file.
			if (!capture_file->is_open()) {
				std::cout << "Capture file not open.\n";
			}
			capture_file->seekg(0, std::ios_base::end);
			std::streamoff chunk_position; 
			chunk_position = capture_file->tellp();
			//std::cout << "adding: chunk_position tell p " << chunk_position << "\n";

			//write chunk to file
			//top-level tag:
			capture_file->write(chunk.tag.c_str(), TAG_SIZE);
			//placeholder for eventual chunk size.
			capture_file->write("0000", RIFF_SIZE);
			//chunk index
			_chunk_index.push_back(FileIndexPt<std::string>(chunk_position, chunk.tag));

			//now, each sub-chunk
			std::map<std::string, std::shared_ptr<AbstractParam>>::iterator param_it;
			std::vector<std::string> param_tags_to_write;
			for (param_it = chunk.params.begin(); param_it != chunk.params.end(); param_it++) {
				param_tags_to_write.push_back(param_it->first);
			}
			//first, write indexing values.
			for (_param_index_it = _param_index.begin(); _param_index_it != _param_index.end(); ++_param_index_it) {
				const std::string indexing_tag = _param_index_it->first;
				int64_t* index_value = chunk.get_parameter_by_tag<int64_t>(indexing_tag);
				if (index_value != nullptr) {
					_param_index[indexing_tag].push_back(FileIndexPt<int64_t>(chunk_position, *index_value));
					//add to disk
					_add_param(chunk, indexing_tag);
					//remove from "to write" list.
					std::vector<std::string>::iterator find_it = std::find(	param_tags_to_write.begin(), 
																			param_tags_to_write.end(), 
																			indexing_tag);
					param_tags_to_write.erase(find_it);
				}
			}

			//next, write remaining values.
			for (int p = 0; p < param_tags_to_write.size(); p++) {
				_add_param(chunk, param_tags_to_write[p]);
			}
			//write chunk size.
			std::streamoff chunk_end_position = capture_file->tellg();
			capture_file->seekg(chunk_position + TAG_SIZE, std::ios_base::beg);
			int chunk_size = chunk_end_position - (chunk_position + TAG_SIZE + RIFF_SIZE);
			capture_file->write(reinterpret_cast<const char*>(&chunk_size), RIFF_SIZE);
			//std::cout << _chunk_index.size() << " - " << chunk_position << " chunk_size " << chunk_size << "\n";
			//go to end again.
			capture_file->seekg(0, std::ios_base::end);
		}

		void _read_chunk_at_file_pos(Chunk* chunk, int64_t file_pos) {
			//std::cout << "_read_chunk_at_file_pos: " << file_pos << "\n";
			//should we be locking the file from other accesses here?
			if (!capture_file->is_open()) {
				std::cout << "Cannot read chunk as capture file is not open.\n";
				return;
			}

			capture_file->seekg(file_pos, std::ios_base::beg);
			//std::cout << "reading chunk @ " << capture_file->tellg() << " \n";

			data_buffer buffer;
			//char* buffer = new char; //2^16 bytes -- overkill?

			//chunk tag
			capture_file->read(buffer.ch_ptr, TAG_SIZE);
			chunk->tag = std::string(buffer.ch_ptr, TAG_SIZE);
			//chunk length
			capture_file->read(buffer.ch_ptr, RIFF_SIZE);
			int chunk_length = buffer.i;

			//read sub-chunks while still inside the chunk->
			while ((std::streamoff)capture_file->tellg() - file_pos < chunk_length + TAG_SIZE + RIFF_SIZE) {
				std::streamoff sub_chunk_start = capture_file->tellg();
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
						chunk->add_parameter_by_tag<int>(sub_tag, buffer.i);
						break;
					case INT_64_TYPE:
						if (sub_chunk_length != sizeof(int64_t))
							std::cout << "sub_chunk_length for int64_t unexpected size" << sub_chunk_length << "\n";
						capture_file->read(buffer.ch_ptr, sizeof(int64_t));
						chunk->add_parameter_by_tag<int64_t>(sub_tag, buffer.i64);
						break;
					case CHAR_PTR_TYPE: 
						{
							char* buffer_ptr = new char[sub_chunk_length];
							capture_file->read(buffer_ptr, sub_chunk_length);
							chunk->add_parameter_by_tag<char*>(sub_tag, buffer_ptr, sub_chunk_length);
						}
						break;
					case UNDEFN_TYPE:
						std::cout << "Found tag of undefined type: " << sub_tag << "\n";
						break;
				}
				//for sanity, move file to start of next expected sub-chunk:
				capture_file->seekg(sub_chunk_start + TAG_SIZE + RIFF_SIZE + sub_chunk_length, std::ios_base::beg);
			}
			//capture_file is now at end of chunk.
		}

		Chunk _read_chunk_at_file_pos(int64_t file_pos) {
			Chunk* chunk = new Chunk();
			_read_chunk_at_file_pos(chunk, file_pos);
			return *chunk;
		}

		Chunk get_at_index(int index) {
			return _read_chunk_at_file_pos(_chunk_index[index].position);
		}

		template <class T>
		void get_at_index_tag(Chunk* chunk, std::string indexing_param_tag, T indexing_value, std::string tag_filter = "") {
			//check parameter index exists.
			_param_index_it = _param_index.find(indexing_param_tag);
			if (_param_index_it == _param_index.end()) {
				std::cout << "We did not index by " << indexing_param_tag << "\n";
				return;
			}

			//find the chunk!
			std::vector<FileIndexPt<int64_t>> param_file_index = _param_index_it->second;
			//do a binary search within param_file_index
			int imax = param_file_index.size() - 1;
			int imin = 0;
			int imid = 0;
			while (imax - imin >= 1)
            {
				imid = (imax - imin) / 2 + imin;
				if (param_file_index[imid].value < indexing_value)
					imin = imid;
				else
					imax = imid;

				//in-between 2 values
				if (imax - imin == 1) {
					if (std::abs(param_file_index[imax].value - param_file_index[imid].value) > 
						std::abs(param_file_index[imid].value - param_file_index[imin].value) ) {
						//imin is closer
						imax = imin; imid = imin;
					} else {
						imin = imax; imid = imax;
					}
				}
			}
			//expect imin == imax
			int64_t file_index = param_file_index[imid].position; 
			_read_chunk_at_file_pos(chunk, file_index);
		}

		template <class T>
		Chunk get_at_index_tag(std::string indexing_param_tag, T indexing_value) {
			Chunk* chunk = new Chunk();;
			get_at_index_tag(chunk, indexing_param_tag, indexing_value);
			return *chunk;
		}

		template <class T>
		void get_at_index(Chunk* chunk, std::string indexing_param, T indexing_value) {
			get_at_index_tag(chunk, tags::get_tag(indexing_param), indexing_value);
		}

		template <class T>
		Chunk get_at_index(std::string indexing_param, T indexing_value) {
			Chunk* chunk = new Chunk();;
			get_at_index(chunk, indexing_param, indexing_value);
			return *chunk;
		}

		void close() {
			std::cout << "closing file " << (void*)capture_file << " .\n";
			capture_file->close();
		}

		~CaptureSession() {
			//close();
		}
	};

};

#endif
