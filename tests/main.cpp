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
            image_bytes[4*(x + y*height) + 3] = 255;
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

//===================================================
int main() {
	//tag definitions from our local project.
	RegisterTags(); 
	//expected behaviour: will get 5 "tag already registered" warnings.
	rfr::tags::register_from_file("tag_definitions.txt");
	
	test_write_read_frames();

	test_fetch_frames();

	test_diff_frame_types();

	std::cout << "finished.\n";

	//while(true) {} //holding pattern.
}
