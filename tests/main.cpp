//Test Code for Riffer

#include "tags.h"
#include "capture_session.h"

//Test helper functions =============================
bool byte_compare(char* array1, char* array2, length) {
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
	rfr::tags::register("colour frame", "CLUR");
	rfr::tags::register("width", "WDTH");
	rfr::tags::register("height", "HGHT");
	rfr::tags::register("timestamp", "MTMP");
	rfr::tags::register("image", "CLRI");
}

//===================================================

int main() {
	//tag definitions from our local project.
	RegisterTags(); 
	//expected behaviour: will get 5 "tag already registered" warnings.
	tags::register_from_file("tag_definitions.txt");
	
	rfr::CaptureSession cs = rfr::CaptureSession("./capture.dat");
	//informs the CaptureSession to index by the "timestamp" tag.
	cs.index_by("timestamp"); 

	//makes new chunk with top-level tag of "colour frame"
	rfr::Chunk chunk = rfr::Chunk("colour frame"); 
	int width = 640; int height = 480;
	//=================CONTINUE WORK HERE==============================================
	chunk.AddParameter(rfr::Param<int>("width", width));
	chunk.AddParameter(rfr::Param<int>("height", height));

	int timestamp = 1234567891011; 
	chunk.AddParameter(rfr::Param<long>("timestamp", 1234567891011));

	//creating random colour image
	char* image_bytes = new char[width*height*4]; //4 bpp
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			char intensity = 255*(x + y)/(width + height); 
		} 	
	}

	//Can we make a template with a pointer?
	chunk.AddParameter(rfr::Param<char*>("image", image_bytes));

	cs.add(chunk);	//writes to disk

	//======chunk_by_index and chunk should be identical
	rfr::Chunk chunk_by_index = cs.GetAtIndex(0);

	assert(width == chunk_by_index.GetParameter<int>("width"));
	assert(height == chunk_by_index.GetParameter<int>("height"));
	assert(timestamp == chunk_by_index.GetParameter<long>("timestamp"));

	assert(byte_compare(image_bytes, chunk_by_index.GetParameter<char*>("image"), width*height*4));

	assert(chunk == chunk_by_index);
	
	//======chunk_by_timestamp and chunk should be identical
	rfr::Chunk chunk_by_timestamp = cs.GetAtIndex("timestamp", timestamp);
	//for the given "timestamp" component type, gets closest to given value.

	assert(width == chunk_by_timestamp.GetParameter<int>("width"));
	assert(height == chunk_by_timestamp.GetParameter<int>("height"));
	assert(timestamp == chunk_by_timestamp.GetParameter<long>("timestamp"));
	assert(nullptr == chunk_by_timestamp.GetParameter<long>("doesn't exist"));

	assert(byte_compare(image_bytes, chunk_by_timestamp.GetParameter<char*>("image"), width*height*4));

	assert(chunk == chunk_by_timestamp);
	
	//======Close and re-open
	cs.close();

	CaptureSession cs_opened = CaptureSession("./capture.dat", false);
	cs_opened.index_by("timestamp");
	cs_opened.run_index();
	rfr::Chunk opened_chunk_by_timestamp = cs.GetAtIndex("timestamp", timestamp);
	assert(chunk == opened_chunk_by_timestamp);
}




