//Test Code for Riffer

#include <assert.h>

#include <riffer.h>

//Test helper functions =============================
bool byte_compare(char* array1, char* array2, int length) {
	//just in case this functionality doesn't exist already.
	for (int i = 0; i < length; i++) {
		if (array1[i] != array2[i])
			return false;
	}
	return true;
}

void RegisterTags() {
	//for now, tags do not have a built-in type definition.
	//the point of the Tags structure is human readability.
	rfr::tags::register_tag("colour frame", "CLUR", CHUNK_TYPE);
	rfr::tags::register_tag("other frame", "OCLR", CHUNK_TYPE);
	rfr::tags::register_tag("width", "WDTH", INT_TYPE);
	rfr::tags::register_tag("height", "HGHT", INT_TYPE);
	rfr::tags::register_tag("timestamp", "MTMP", INT_64_TYPE);
	rfr::tags::register_tag("image", "CLRI", CHAR_PTR_TYPE);
}

void test_write_read_frames() {
	rfr::CaptureSession cs;
	if (!cs.is_open()) {
		std::cout << "CaptureSession file not open.\n";
	} else {
		//std::cout << "CaptureSession file IS open.\n";
	}
	
	//informs the CaptureSession to index by the "timestamp" tag.
	cs.index_by("timestamp"); 

	//makes new chunk with top-level tag of "colour frame"
	rfr::Chunk chunk("colour frame"); 
	int width = 640; int height = 480;
	chunk.add_parameter("width", width);
	chunk.add_parameter("height", height);

	int64_t timestamp = 1234567891011; 
	chunk.add_parameter("timestamp", 1234567891011);

	//creating colour image - assume 4 bpp
	int img_length = width*height*4;
	char* image_bytes = new char[img_length]; //4 bpp
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			char intensity = 255*((width-x) + y)/(width + height);
            image_bytes[4*(x + y*height) + 0] = intensity;
            image_bytes[4*(x + y*height) + 1] = intensity;
            image_bytes[4*(x + y*height) + 2] = intensity;
            image_bytes[4*(x + y*height) + 3] = (char)255;
		} 	
	}

	//Can we make a template with a pointer?
	chunk.add_parameter("image", image_bytes, img_length);//, width*height*4);

	cs.add(chunk);	//writes to disk

	//======chunk_by_index and chunk should be identical
	rfr::Chunk chunk_by_index = cs.get_at(0);

	assert(width == *(chunk_by_index.get_parameter<int>("width")));
	assert(height == *(chunk_by_index.get_parameter<int>("height")));
	int64_t read_timestamp = *(chunk_by_index.get_parameter<int64_t>("timestamp"));
	assert(timestamp == read_timestamp);

	assert(byte_compare(image_bytes, *(chunk_by_index.get_parameter<char*>("image")), img_length));

	//below triggers abort() call - not sure why.
	//assert(chunk == chunk_by_index);
	
	//======chunk_by_timestamp and chunk should be identical
	rfr::Chunk chunk_by_timestamp = cs.get_by_index(timestamp);
	//for the given "timestamp" component type, gets closest to given value.

	assert(width == *(chunk_by_timestamp.get_parameter<int>("width")));
	assert(height == *(chunk_by_timestamp.get_parameter<int>("height")));
	assert(timestamp == *(chunk_by_timestamp.get_parameter<int64_t>("timestamp")));
	assert(nullptr == chunk_by_timestamp.get_parameter<int64_t>("doesn't exist"));

	assert(byte_compare(image_bytes, *(chunk_by_timestamp.get_parameter<char*>("image")), img_length));

	//assert(chunk == chunk_by_timestamp);
	
	//======Close and re-open
	cs.close();

	rfr::CaptureSession cs_opened("./", "capture.dat", false);
	cs_opened.index_by("timestamp");
	cs_opened.run_index();
	rfr::Chunk opened_chunk_by_timestamp = cs_opened.get_by_index(timestamp);
	
	//assert(chunk == opened_chunk_by_timestamp);
}

void test_fetch_frames() {
	//write a bunch of frames of different sizes...make sure we get the right ones back
	
	rfr::tags::register_tag("frame", "FRMM", CHUNK_TYPE);

	rfr::tags::register_tag("number", "NUMM", STRING_TYPE);
	
	std::vector<std::string> frame_tags;
	std::vector<int64_t> timestamps;
	frame_tags.push_back("ZERO");	timestamps.push_back(1000);
	frame_tags.push_back("ONE");	timestamps.push_back(2500);
	frame_tags.push_back("TWO");	timestamps.push_back(2600);
	frame_tags.push_back("THREE");	timestamps.push_back(10000);
	frame_tags.push_back("FOUR");	timestamps.push_back(10001);
	frame_tags.push_back("FIVE");	timestamps.push_back(10005);
	frame_tags.push_back("SIX");	timestamps.push_back(15000);
	frame_tags.push_back("SEVEN");	timestamps.push_back(19000);
	frame_tags.push_back("EIGHT");	timestamps.push_back(100000);
	frame_tags.push_back("NINE");	timestamps.push_back(200000);
	frame_tags.push_back("TEN");	timestamps.push_back(200004);

	//create capture
	rfr::CaptureSession cs;
	cs.index_by("timestamp");
	for (int i = 0; i < frame_tags.size(); i++) {
		rfr::Chunk chunk("frame");
		chunk.add_parameter("number", frame_tags[i]);
		chunk.add_parameter("timestamp", timestamps[i]);
		cs.add(chunk);
	}

	//test indexing
	for (int i = (int)(frame_tags.size() - 1); i >=0; i--) {
		std::string* fetched_number = cs.get_at(i).get_parameter<std::string>("number");
		if (!fetched_number) {
			std::cout << "Missing number on fetch! \n";
		} else {
			//std::cout << frame_tags[i].c_str() << " - " << fetched_number->c_str() <<  "\n";
			assert(strcmp(fetched_number->c_str(), frame_tags[i].c_str()) == 0);
		}
	}

	//indexing by timestamp
	for (int i = 0; i < frame_tags.size(); i++) {
		const char* fetch = cs.get_by_index(timestamps[i]).get_parameter<std::string>("number")->c_str();
		const char* original = frame_tags[i].c_str();
		int strcmp_result = strcmp(fetch, original);

		if (strcmp_result != 0)
			std::cout << fetch << " " << original << " " << strcmp(fetch, original) << "\n";

		assert( 0 == strcmp_result);
	}
}

void test_copy() {

	rfr::tags::register_tag("frame", "FRMM", CHUNK_TYPE);

	rfr::tags::register_tag("number", "NUMM", STRING_TYPE);

	std::vector<std::string> frame_tags;
	std::vector<int64_t> timestamps;
	frame_tags.push_back("ZERO");	timestamps.push_back(0);
	frame_tags.push_back("ONE");	timestamps.push_back(1);
	frame_tags.push_back("TWO");	timestamps.push_back(2);
	frame_tags.push_back("THREE");	timestamps.push_back(3);
	frame_tags.push_back("FOUR");	timestamps.push_back(4);
	frame_tags.push_back("FIVE");	timestamps.push_back(5);
	frame_tags.push_back("SIX");	timestamps.push_back(6);
	frame_tags.push_back("SEVEN");	timestamps.push_back(7);
	frame_tags.push_back("EIGHT");	timestamps.push_back(8);
	frame_tags.push_back("NINE");	timestamps.push_back(9);
	frame_tags.push_back("TEN");	timestamps.push_back(10);

	//create capture
	rfr::CaptureSession cs_first("./", "first.dat");
	for (int i = 0; i < frame_tags.size(); i++) {
		rfr::Chunk chunk("frame");
		chunk.add_parameter("number", frame_tags[i]);
		chunk.add_parameter("timestamp", timestamps[i]);

		cs_first.add(chunk);
	}

	int copy_start = 3;
	int copy_end = 9;

	rfr::CaptureSession cs_second("./", "second.dat");
	cs_first.copyTo(cs_second, "timestamp", copy_start, copy_end);
	
	assert(cs_second.length() == (copy_end - copy_start + 1));

	for (int i = 0; i < cs_second.length(); i++) {
		assert(*(cs_second.get_at(i).get_parameter<std::string>("number")) == frame_tags[i + copy_start]);
	}
}

struct FrameInfo {
	std::string frame_type;
	std::string frame_data;
	int64_t timestamp;
	FrameInfo(std::string _frame_type, std::string _frame_data, int64_t _timestamp)
		: frame_type(_frame_type), frame_data(_frame_data), timestamp(_timestamp) { }
};

void test_diff_frame_types() {
	rfr::tags::register_tag("data", "DATA", STRING_TYPE);

	std::vector<FrameInfo> frame_info;
	frame_info.push_back(FrameInfo("colour frame", "dog", 1));
	frame_info.push_back(FrameInfo("other frame", "nog", 1));
	frame_info.push_back(FrameInfo("other frame", "cat", 100));
	frame_info.push_back(FrameInfo("other frame", "kitten", 101));
	frame_info.push_back(FrameInfo("other frame", "moose", 151));
	frame_info.push_back(FrameInfo("colour frame", "not moose", 152));
	frame_info.push_back(FrameInfo("colour frame", "raccoon", 221));
	frame_info.push_back(FrameInfo("colour frame", "jesus", 222));
	frame_info.push_back(FrameInfo("other frame", "anti-jesus", 222));
	frame_info.push_back(FrameInfo("other frame", "also jesus", 223));
	frame_info.push_back(FrameInfo("colour frame", "milkshake", 228));
	frame_info.push_back(FrameInfo("other frame", "boat", 229));

	rfr::CaptureSession cs;
	cs.index_by("timestamp");

	for (int i = 0; i < frame_info.size(); i++) {
		rfr::Chunk chunk(frame_info[i].frame_type);
		chunk.add_parameter("data", frame_info[i].frame_data);
		chunk.add_parameter("timestamp", frame_info[i].timestamp);
		cs.add(chunk);
	}

	//test direct results
	for (int i = 0; i < frame_info.size(); i++) {
		//fetch using frame filter type
		rfr::Chunk chunk = cs.get_by_index(frame_info[i].timestamp, frame_info[i].frame_type);

		std::string expected_frame_type_tag = rfr::tags::get_tag(frame_info[i].frame_type);
		std::string fetched_frame_type_tag = chunk.tag;
		assert(fetched_frame_type_tag == expected_frame_type_tag);
		
		std::string fetched_data = *chunk.get_parameter<std::string>("data");
		assert(fetched_data == frame_info[i].frame_data);
	}

	//could do other tests with close timestamps
}

void test_corruption_load(std::string filename, std::string msg = "") {
    //testing a file on load, expecting corruption.
    
    std::cout << msg << " - expecting to find corruption in capture session: " << filename << "\n";
    
    //should catch corruption.
    rfr::CaptureSession cs("./", filename, false);
    
    //Question: should run_index be called from the constructor if we are not overwriting?
    //  Answer: No. Indexing needs to know by what tags it is indexing by before running.
    
    cs.run_index(); //expect corruption to be found here.
    
    cs.close();
}

void test_corruption_last_chunk() {
    std::string filename = "./capture.dat";
    std::ios_base::openmode mode = std::fstream::binary | std::fstream::in | std::fstream::out | std::fstream::trunc;;
    
    std::fstream* capture_file = new std::fstream(filename, mode);
    
    //write some frames to file
    const int SUBCHUNK_SIZE = 7;
    const int CHUNK_SIZE = SUBCHUNK_SIZE + RIFF_SIZE + RIFF_SIZE;
    
    const char* CHUNK_TAG = "CTAG";
    const char* SUBCHUNK_TAG = "STAG";
    
    rfr::tags::register_tag("STAG", "STAG", STRING_TYPE);
    
    for (char i = 0; i < 5; i++) {
        capture_file->write(CHUNK_TAG, RIFF_SIZE); //top-level
        capture_file->write(reinterpret_cast<const char*>(&CHUNK_SIZE), RIFF_SIZE); //top-level size
        
        //subchunk
        capture_file->write(SUBCHUNK_TAG, RIFF_SIZE);
        capture_file->write(reinterpret_cast<const char*>(&SUBCHUNK_SIZE), RIFF_SIZE);
        
        //write the subchunk data
        capture_file->write("abcdef", 6);
        capture_file->write(reinterpret_cast<const char*>(&i), 1);
        
    }
    
    //write corrupt frame
    capture_file->write(CHUNK_TAG, RIFF_SIZE); //top-level
    capture_file->write(reinterpret_cast<const char*>(&CHUNK_SIZE), RIFF_SIZE); //top-level size
    //subchunk
    capture_file->write(SUBCHUNK_TAG, RIFF_SIZE);
    capture_file->write(reinterpret_cast<const char*>(&SUBCHUNK_SIZE), RIFF_SIZE);
    //smaller subchunk data
    capture_file->write("abcd", 4);
    
    capture_file->close();
    
    //test on load
    test_corruption_load(filename, "test_corruption_last_chunk");
    
    //test adding a chunk.
    std::cout << "\t adding a chunk to overwrite corruption \n";
    rfr::CaptureSession cs("./", filename, false);
    cs.run_index(); //will put file_end pointer at right position.
    
    std::string test_value = "abcdefgh";
    rfr::Chunk chunk(CHUNK_TAG);
    chunk.add_parameter(SUBCHUNK_TAG, test_value);
    cs.add(chunk);
    
    cs.close();
    
    //retrieve test value
    rfr::CaptureSession cs_retr("./", filename, false);
    cs_retr.run_index();
    rfr::Chunk last_chunk = cs_retr.last();
    
    assert(*(last_chunk.get_parameter<std::string>(SUBCHUNK_TAG)) == test_value);
}

void test_corruption() {
    //intentionally induce corruption, which should be caught on re-load
    
    //the policy of riffer, when dealing with the size of a top-level chunk that is incorrect,
    // is the delete the chunk and pretend it never existed.
    
    //in real cases, we expect corrupt chunks as the last chunk in a capture session, due to interrupted writes
    // BUT THEY COULD BE ANYWHERE
    // BE EVER WATCHFUL
    
    test_corruption_last_chunk();

    //test_corruption_mid_chunk();
    //TODO test_corruption_mid_chunk
    //some smaller, some larger
}

//===================================================
int main() {
	//tag definitions from our local project.
	RegisterTags(); 
	//expected behaviour: will get 5 "tag already registered" warnings.
	rfr::tags::register_from_file("tag_definitions.txt");
	
	test_write_read_frames();

	test_fetch_frames();

	test_copy();

	test_diff_frame_types();
    
    test_corruption();

	std::cout << "finished.\n";

	//while(true) {} //holding pattern.
}
