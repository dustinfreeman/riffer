//Test Code for Riffer

#include <assert.h>

#include <tags.h>
#include <capture_session.h>

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
	rfr::tags::register_tag("colour frame", "CLUR", CHAR_PTR_TYPE);
	rfr::tags::register_tag("width", "WDTH", INT_TYPE);
	rfr::tags::register_tag("height", "HGHT", INT_TYPE);
	rfr::tags::register_tag("timestamp", "MTMP", LONG_TYPE);
	rfr::tags::register_tag("image", "CLRI", INT_TYPE);
}

//===================================================

int main() {
	//tag definitions from our local project.
	RegisterTags(); 
	//expected behaviour: will get 5 "tag already registered" warnings.
	rfr::tags::register_from_file("tag_definitions.txt");
	
	rfr::CaptureSession cs = rfr::CaptureSession("./capture.dat");
	//informs the CaptureSession to index by the "timestamp" tag.
	cs.index_by("timestamp"); 

	//makes new chunk with top-level tag of "colour frame"
	rfr::Chunk chunk = rfr::Chunk("colour frame"); 
	int width = 640; int height = 480;
	//=================CONTINUE WORK HERE==============================================
	chunk.add_parameter("width", width);
	chunk.add_parameter("height", height);

	int timestamp = 1234567891011; 
	chunk.add_parameter("timestamp", 1234567891011);

	//creating colour image
	char* image_bytes = new char[width*height*4]; //4 bpp
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			char intensity = 255*(x + y)/(width + height); 
		} 	
	}

	//Can we make a template with a pointer?
	chunk.add_parameter("image", image_bytes);

	cs.add(chunk);	//writes to disk

	//======chunk_by_index and chunk should be identical
	rfr::Chunk chunk_by_index = cs.get_at_index(0);

	assert(width == *chunk_by_index.get_parameter<int>("width"));
	assert(height == *chunk_by_index.get_parameter<int>("height"));
	assert(timestamp == *chunk_by_index.get_parameter<long>("timestamp"));

	assert(byte_compare(image_bytes, *chunk_by_index.get_parameter<char*>("image"), width*height*4));

	assert(chunk == chunk_by_index);
	
	//======chunk_by_timestamp and chunk should be identical
	rfr::Chunk chunk_by_timestamp = cs.get_at_index("timestamp", timestamp);
	//for the given "timestamp" component type, gets closest to given value.

	assert(width == *chunk_by_timestamp.get_parameter<int>("width"));
	assert(height == *chunk_by_timestamp.get_parameter<int>("height"));
	assert(timestamp == *chunk_by_timestamp.get_parameter<long>("timestamp"));
	assert(0 == chunk_by_timestamp.get_parameter<long>("doesn't exist"));
	//TODO above error returns a default value...this feels wrong.

	assert(byte_compare(image_bytes, *chunk_by_timestamp.get_parameter<char*>("image"), width*height*4));

	assert(chunk == chunk_by_timestamp);
	
	//======Close and re-open
	cs.close();

	rfr::CaptureSession cs_opened = rfr::CaptureSession("./capture.dat", false);
	cs_opened.index_by("timestamp");
	cs_opened.run_index();
	rfr::Chunk opened_chunk_by_timestamp = cs.get_at_index("timestamp", timestamp);
	assert(chunk == opened_chunk_by_timestamp);
}




