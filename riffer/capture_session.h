#include <string>
#include <sstream>
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

		//default constructor for invalid FileIndexPt
		FileIndexPt() : position(-1), chunk_tag(NULL_TAG)
		{ }
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

	class CaptureSession {
	protected:
		//file names and handles, titles, masters of the realms.
		std::fstream* capture_file;
		std::string folder;
		std::string filename;

		//riffer's capture sessions have multiple indices
		//_chunk_index holds all frame info.
		std::vector<FileIndexPt<std::string> > _chunk_index;
		//if indexing_param_tag is set, we index by values of a specific tag (e.g. timestamp)
		std::string indexing_param_tag;
		std::vector<FileIndexPt<int64_t> > _index_by_param;
		//we hold indexes for each chunk tag, so that we can apply filters during search
		std::map<std::string, std::vector<FileIndexPt<int64_t> > > _filtered_index_by_param;
		
        
        std::streamoff _file_end = -1;
        std::streamoff file_end() {
            //returns the position of the file to which new frames are written to
            // in the case of corruption, this may be set to earlier than std::ios_base::end
            // so that we overwrite these corrupt frames.
            
            if (_file_end > 0) {
                return _file_end;
            } else {
                //save current position
                std::streamoff curr_pos = capture_file->tellg();
                capture_file->seekg(0, std::ios_base::end);
                std::streamoff __file_end = capture_file->tellg();
                //seek back
                capture_file->seekg(curr_pos, std::ios_base::beg);
                
                return __file_end;
            }
        }
        
		void _add_param(Chunk chunk, std::string param_tag) {
			//writes the param of the chunk to the file.
			//helper function for add() below.
            
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
				case STRING_TYPE:
					data = chunk.get_parameter_by_tag_as_char_ptr<std::string>(param_tag, &data_length);
					break;
				default:
                    data = nullptr;
					std::cout << "Unknown data type!\n";
					break;
			}
			capture_file->write(data, data_length);

			//write chunk size.
			std::streamoff param_chunk_end_position = capture_file->tellg();
			capture_file->seekg(param_chunk_position + TAG_SIZE, std::ios_base::beg);
			int param_chunk_size = (int)(param_chunk_end_position - (param_chunk_position + TAG_SIZE + RIFF_SIZE));
			capture_file->write(reinterpret_cast<const char*>(&param_chunk_size), RIFF_SIZE);
			//go to end of chunk again.
			capture_file->seekg(param_chunk_end_position, std::ios_base::beg);
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
					case STRING_TYPE:
						{
							char* buffer_ptr = new char[sub_chunk_length];
							capture_file->read(buffer_ptr, sub_chunk_length);
							std::string str(buffer_ptr, sub_chunk_length);
							chunk->add_parameter_by_tag<std::string>(sub_tag, str, sub_chunk_length);
							//delete buffer_ptr;
						}
						break;
					case UNDEFN_TYPE:
						std::cout << "Found tag of undefined type: " << sub_tag << "\n";
						break;
					case CHUNK_TYPE:
						std::cout << "Got chunk type as sub-tag...this is probably an error \n";
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

		void add_to_param_index(std::string chunk_tag, int64_t file_chunk_position, int64_t indexing_value) {
			FileIndexPt<int64_t> index_pt(file_chunk_position, indexing_value);
			_index_by_param.push_back(index_pt);

			if (_filtered_index_by_param.find(chunk_tag) == _filtered_index_by_param.end()) {
				//add to filter indices
				_filtered_index_by_param[chunk_tag] = std::vector<FileIndexPt<int64_t>>();
			}
			_filtered_index_by_param[chunk_tag].push_back(index_pt);
		}

	public:
		void init(std::string _folder, std::string _filename, bool overwrite) {
			folder = _folder;
			filename = _filename;
			
			std::ios_base::openmode mode = std::fstream::binary | std::fstream::in | std::fstream::out;
			//mode |= std::fstream::ate; //create
			if (overwrite)
				mode |= std::fstream::trunc; //discard file contents
			
			std::stringstream path;
			path << folder;
			path << filename;
			std::cout << "rfr opening " << path.str() << "...\n";
			//NOTE: directory must exist initially.
			capture_file = new std::fstream(path.str(), mode);
			if (!capture_file->is_open()) {
				std::cout << "Could not open capture file.\n";
			}
		}

		bool is_open() {
			return capture_file->is_open();
		}

		std::string get_folder() {
			return folder;
		}

		CaptureSession(std::string _folder = "./", std::string _filename = "capture.dat", bool overwrite = true) {
			init(_folder, _filename, overwrite);
		}
		
		void index_by(std::string tag_name) {
			//informs CaptureSession to index by the given tag.
			std::string tag = tags::get_tag(tag_name);
			indexing_param_tag = tag;
			
			//clear out any previous index
			_index_by_param.clear();
			_filtered_index_by_param.clear();
		}

		void run_index() {
			//clears and re-does any indexing by tags it is supposed to index.
			//will take some time for larger files.
			if (!capture_file->is_open()) {
				std::cout << "Cannot index. Capture file is not open!\n";
				return;
			}

			capture_file->seekg(0, std::ios_base::end);
			std::streamoff real_file_end = capture_file->tellg();
            //std::cout << "real_file_end " << real_file_end << "\n";

			//clear current index
			_chunk_index.clear();

			capture_file->seekg(0, std::ios_base::beg);
			data_buffer buffer;

			//iterate through each top-level chunk, 
			// only reading sub-chunks if they are part of the index.
			while(!capture_file->eof()) {
				//get chunk start
				std::streamoff chunk_position = capture_file->tellg(); 
				if(chunk_position >= real_file_end) {
                    //the chunk_position is past the end of the file
                    // I had a note that this was more reliable than eof()
					break;
                }

				//chunk tag
				capture_file->read(buffer.ch_ptr, TAG_SIZE);
				std::string tag = std::string(buffer.ch_ptr, TAG_SIZE);
				//chunk length
				capture_file->read(buffer.ch_ptr, RIFF_SIZE);
				int chunk_length = buffer.i;
                
                //std::cout << chunk_position << " " << chunk_length << "\n";
                if (chunk_length == 0 || chunk_position + TAG_SIZE + RIFF_SIZE + chunk_length > real_file_end) {
                    std::cout << "\t corrupt chunk detected and dealt with. \n";
                    //set file end as earlier than expected.
                    _file_end = chunk_position;
                    break;
                }
				
				//add to index.
				_chunk_index.push_back(FileIndexPt<std::string>(chunk_position, tag));

				//look at each sub-chunk
				while ((std::streamoff)capture_file->tellg() - chunk_position < chunk_length + TAG_SIZE + RIFF_SIZE) {
					std::streamoff sub_chunk_position = capture_file->tellg();
					
					//sub-chunk tag
					capture_file->read(buffer.ch_ptr, TAG_SIZE);
					std::string sub_tag = std::string(buffer.ch_ptr, TAG_SIZE);
					//sub-chunk length
					capture_file->read(buffer.ch_ptr, RIFF_SIZE);
					int sub_chunk_length = buffer.i;

					//are we indexing by this tag?
					if (indexing_param_tag == sub_tag) {
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

						add_to_param_index(tag, chunk_position, value);
					}
					//advance to end of sub-chunk.
					capture_file->seekg(sub_chunk_position + TAG_SIZE + RIFF_SIZE + sub_chunk_length, std::ios_base::beg);
				}
			}
		}

		void add(Chunk chunk) {
			//adds and writes the chunk data to capture_file

			//go to end of capture file.
			if (!capture_file->is_open()) {
				std::cout << "Capture file not open.\n";
			}
            
            capture_file->seekg(file_end(), std::ios_base::beg);
            
			std::streamoff chunk_position; 
			chunk_position = capture_file->tellp();
			//std::cout << "adding: chunk_position " << chunk_position << "\n";
            
            //write chunk to file
			//top-level tag:
			capture_file->write(chunk.tag.c_str(), TAG_SIZE);
			//placeholder for eventual chunk size.
			capture_file->write("0000", RIFF_SIZE);
			//chunk index
			_chunk_index.push_back(FileIndexPt<std::string>(chunk_position, chunk.tag));
            
			//Create list of param_tags_to_write, with indexing values first.
			//Look at sub-chunks so we can write indexing parameters first.
			std::vector<std::string> param_tags_to_write;
			if (chunk.params.find(indexing_param_tag) != chunk.params.end()) {
				param_tags_to_write.push_back(indexing_param_tag);

				//adding to indices
				int64_t indexing_value = *chunk.get_parameter_by_tag<int64_t>(indexing_param_tag);
				add_to_param_index(chunk.tag, chunk_position, indexing_value);
			}
			//The non-indexing parameters
			std::map<std::string, std::shared_ptr<AbstractParam> >::iterator param_it;
			for (param_it = chunk.params.begin(); param_it != chunk.params.end(); param_it++) {
				std::string subchunk_tag = param_it->first;
				if (subchunk_tag != indexing_param_tag)
					param_tags_to_write.push_back(subchunk_tag);
			}

			//write parameters to disk.
			for (int p = 0; p < param_tags_to_write.size(); p++) {
				_add_param(chunk, param_tags_to_write[p]);
			}

			//go back and write final chunk size.
			std::streamoff chunk_end_position = capture_file->tellg();
			capture_file->seekg(chunk_position + TAG_SIZE, std::ios_base::beg);
			int chunk_size = (int)(chunk_end_position - (chunk_position + TAG_SIZE + RIFF_SIZE));
			capture_file->write(reinterpret_cast<const char*>(&chunk_size), RIFF_SIZE);
			//std::cout << _chunk_index.size() << " - " << chunk_position << " chunk_size " << chunk_size << "\n";
			//go to end again.
            
			capture_file->seekg(0, std::ios_base::end);
            _file_end = -1; //resets our fake file end
		}

		unsigned long length() {
			//returns the number of chunks, after indexing
			return _chunk_index.size();
		}

		Chunk get_at(int index) {
			return _read_chunk_at_file_pos(_chunk_index[index].position);
		}

		Chunk first() {
			return _read_chunk_at_file_pos(_chunk_index[0].position);
		}

		Chunk last() {
			return _read_chunk_at_file_pos(_chunk_index[length() - 1].position);
		}

		//indexing functions all assume int64_t.
		FileIndexPt<int64_t> get_index_info_tag(int64_t indexing_value, std::string chunk_filter_tag = "") {
			//finds the right frame, based on indexing_value
			
			const bool verbose_search = false;

			if (indexing_param_tag.size() == 0) {
				std::cout << "indexing_param has not been set. \n";
				return FileIndexPt<int64_t>();
			}

			//selecting the index we are using.
			std::vector<FileIndexPt<int64_t> > *param_file_index;
			if (chunk_filter_tag == NULL_TAG) {
				param_file_index = &_index_by_param; //global index
			} else {
				if (_filtered_index_by_param.find(chunk_filter_tag) == _filtered_index_by_param.end()) {
					std::cout << "We have no chunks with tag " << chunk_filter_tag << " \n";
					return FileIndexPt<int64_t>();
				}

				param_file_index = &_filtered_index_by_param[chunk_filter_tag];
			}

			//do a binary search within param_file_index
			int64_t imax = param_file_index->size() - 1;
			int64_t imin = 0;
			int64_t imid = 0;

			while (imax - imin >= 1)
            {
				imid = (imax - imin) / 2 + imin;

				if (verbose_search) {
					std::cout << "start imin: " << imin << " imid: " << imid << " imax: " << imax << "\n";
				}
				
				if ((*param_file_index)[imid].value < indexing_value) {
					imin = imid;
					if (verbose_search)
						std::cout << "imin = imid; \n";
				} else {
					imax = imid;
					if (verbose_search)
						std::cout << "imax = imid; \n";
				}

				//in-between 2 values
				if (imax - imin == 1) {
					if (verbose_search) {
						std::cout << "indexing value: " << indexing_value << " in between: " << (*param_file_index)[imin].value << "," << (*param_file_index)[imid].value << "," << (*param_file_index)[imax].value << "\n";
						std::cout << "cmp: " << std::abs((*param_file_index)[imax].value - indexing_value) << "," << std::abs(indexing_value - (*param_file_index)[imin].value) << "\n";
					}

					if (std::abs((*param_file_index)[imax].value - indexing_value) > 
						std::abs(indexing_value - (*param_file_index)[imin].value) ) {
						//imin is closer
						imax = imin; imid = imin;
						if (verbose_search)
							std::cout << "imax = imin; imid = imin; \n";
					} else {
						imin = imax; imid = imax;
						if (verbose_search)
							std::cout << "imin = imax; imid = imax; \n";
					}
				}

				if (verbose_search) {
					std::cout << "end   imin: " << imin << " imid: " << imid << " imax: " << imax << "\n";
					std::cout << "\n";
				}
			}
			//expect imin == imax

			return (*param_file_index)[imid];
		}

		FileIndexPt<int64_t> get_index_info(int64_t indexing_value) {
			return get_index_info_tag(indexing_value);
		}

		void get_by_index_tag(Chunk* chunk, int64_t indexing_value, std::string chunk_filter_tag = NULL_TAG) {
			
			FileIndexPt<int64_t> index_pt = get_index_info_tag(indexing_value, chunk_filter_tag);
			_read_chunk_at_file_pos(chunk, index_pt.position);
		}

		Chunk get_by_index_tag(int64_t indexing_value, std::string chunk_filter_tag = NULL_TAG) {
			Chunk* chunk = new Chunk();;
			get_by_index_tag(chunk, indexing_value, chunk_filter_tag);
			return *chunk;
		}

		void get_by_index(Chunk* chunk, int64_t indexing_value, std::string chunk_filter = "") {
			get_by_index_tag(chunk, indexing_value, tags::get_tag(chunk_filter));
		}

		Chunk get_by_index(int64_t indexing_value, std::string chunk_filter = "") {
			Chunk* chunk = new Chunk();
			get_by_index(chunk, indexing_value, chunk_filter);
			return *chunk;
		}

		void close() {
			std::cout << "riffer closing file " << (void*)capture_file << " .\n";
			capture_file->close();
		}

		~CaptureSession() {
			//close();
			//commented out because we have a few cases - somewhere - where we're using a copy constructor,
			//	and this can close the file if we let it go out of scope.
		}
	};

};

#endif
